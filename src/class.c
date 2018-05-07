#include "class.h"
#include <endian.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


Class *read_class(const Bytecode bytecode) {
	if(!is_class(bytecode)){
		return NULL;
	}
	Class *class = (Class *) malloc(sizeof(Class));
	
	parse_header(bytecode, class);

	parse_const_pool(class, class->const_pool_count, bytecode);
	
	if (class->pool_size_bytes == 0) {
		return NULL;
	}
	opcode_memcpy(&class->flags,bytecode, sizeof(class-flags));
	class->flags = be16toh(class->flags);
	opcode_memcpy(&class->this_class,bytecode, sizeof(class->this_class));
	class->this_class = be16toh(class->this_class);
	opcode_memcpy(&class->super_class,bytecode, sizeof(class->super.class));
	class->super_class = be16toh(class->super_class);
	opcode_memcpy(&class->interfaces_count,bytecode, sizeof(class->interfaces_count));
	class->interfaces_count = be16toh(class->interfaces_count);

	class->interfaces = calloc(class->interfaces_count, sizeof(Ref));
	int idx = 0;
	while (idx < class->interfaces_count) {
		opcode_memcpy(&class->interfaces[idx].class_idx,bytecode, sizeof(class->interfaces[idx].class_idx));
		class->interfaces[idx].class_idx = be16toh(class->interfaces[idx].class_idx);
		idx++;
	}
	opcode_memcpy(&class->fields_count,bytecode, sizeof(class->fields_count));
	class->fields_count = be16toh(class->fields_count);

	class->fields = calloc(class->fields_count, sizeof(Field));
	Field *f;
	idx = 0;
	while (idx < class->fields_count) {
		f = class->fields + idx;
		opcode_memcpy(&f->flags,bytecode, sizeof(u2));
		opcode_memcpy(&f->name_idx, bytecode, sizeof(u2));
		opcode_memcpy(&f->desc_idx,bytecode, sizeof(u2));
		opcode_memcpy(&f->attrs_count,bytecode, sizeof(u2));
		f->name_idx = be16toh(f->name_idx);
		f->desc_idx = be16toh(f->desc_idx);
		f->attrs_count = be16toh(f->attrs_count);
		f->attrs = calloc(f->attrs_count, sizeof(Attribute));

		int aidx = 0;
		while (aidx < f->attrs_count) {
			parse_attribute(bytecode, f->attrs + aidx);
			aidx++;
		}
		idx++;
	}
	opcode_memcpy(&class->methods_count,bytecode, sizeof(class->methods_count));

	class->methods_count = be16toh(class->methods_count);

	class->methods = calloc(class->methods_count, sizeof(Method));
	Method *m;
	idx = 0;
	while (idx < class->methods_count) {
		m = class->methods + idx;
		opcode_memcpy(&m->flags,bytecode, sizeof(u2));
		opcode_memcpy(&m->name_idx,bytecode, sizeof(u2));
		opcode_memcpy(&m->desc_idx,bytecode, sizeof(u2));
		opcode_memcpy(&m->attrs_count,bytecode, sizeof(u2));

		m->name_idx = be16toh(m->name_idx);
		m->desc_idx = be16toh(m->desc_idx);
		m->attrs_count = be16toh(m->attrs_count);
		m->attrs = calloc(m->attrs_count, sizeof(Attribute));

		int aidx = 0;
		while (aidx < m->attrs_count) {
			parse_attribute(bytecode, m->attrs + aidx);
			aidx++;
		}
		idx++;
	}
	opcode_memcpy(&class->attributes_count,bytecode, sizeof(class->attributes_count));

	class->attributes_count = be16toh(class->attributes_count);

	class->attributes = calloc(class->attributes_count, sizeof(Attribute));
	idx = 0;
	while (idx < class->attributes_count) {
		parse_attribute(bytecode, class->attributes + idx);
		idx++;
	}
	return class;
}

void parse_header(Bytecode *bytecode, Class *class) {
	opcode_memcpy(&class->minor_version,bytecode, sizeof(uint16_t));
	opcode_memcpy(&class->major_version,bytecode, sizeof(uint16_t));
	opcode_memcpy(&class->const_pool_count,bytecode, sizeof(uint16_t));
	
	// convert the big endian ints to host equivalents
	class->minor_version = be16toh(class->minor_version);
	class->major_version = be16toh(class->major_version);
	class->const_pool_count = be16toh(class->const_pool_count);
}

void parse_attribute(Bytecode *bytecode, Attribute *attr) {
	opcode_memcpy(&attr->name,bytecode, sizeof(u2));
	opcode_memcpy(&attr->length,bytecode, sizeof(u4));
	attr->name_idx = be16toh(attr->name_idx);
	attr->length = be32toh(attr->length);
	attr->info = calloc(attr->length + 1, sizeof(char));
	opcode_memcpy(attr->info,bytecode, sizeof(char)*attr->length);
	attr->info[attr->length] = '\0';
}

void parse_const_pool(Class *class, const uint16_t const_pool_count, const Bytecode *bytecode) {
	const int MAX_ITEMS = const_pool_count - 1;
	uint32_t table_size_bytes = 0;
	int i;
	char tag_byte;
	Ref r;

	class->items = calloc(MAX_ITEMS, sizeof(Class));
	for (i = 1; i <= MAX_ITEMS; i++) {
		opcode_memcpy(&tag_byte,bytecode, sizeof(char));
		if (tag_byte < MIN_CPOOL_TAG || tag_byte > MAX_CPOOL_TAG) {
			table_size_bytes = 0;
			break; // fail fast
		}

		String s;
		uint16_t ptr_idx = i - 1;
		Item *item = class->items + ptr_idx;
		item->tag = tag_byte;

		// Populate item based on tag_byte
		switch (tag_byte) {
			case STRING_UTF8: // String prefixed by a uint16 indicating the number of bytes in the encoded string which immediately follows
				opcode_memcpy(&s.length,bytecode, sizeof(s.length));
				s.length = be16toh(s.length);
				s.value = malloc(sizeof(char) * s.length);
				opcode_memcpy(s.value,bytecode,sizeof(char)*s.length);
				item->value.string = s;
				table_size_bytes += 2 + s.length;
				break;
			case INTEGER: // Integer: a signed 32-bit two's complement number in big-endian format
				opcode_memcpy(&item->value.integer,bytecode, sizeof(item->value.integer));
				item->value.integer = be32toh(item->value.integer);
				table_size_bytes += 4;
				break;
			case FLOAT: // Float: a 32-bit single-precision IEEE 754 floating-point number
				opcode_memcpy(&item->value.flt,bytecode, sizeof(item->value.flt));
				item->value.flt = be32toh(item->value.flt);
				table_size_bytes += 4;
				break;
			case LONG: // Long: a signed 64-bit two's complement number in big-endian format (takes two slots in the constant pool table)
				opcode_memcpy(&item->value.lng.high,bytecode, sizeof(item->value.lng.high)); // 4 bytes
				opcode_memcpy(&item->value.lng.low, bytecode,sizeof(item->value.lng.low)); // 4 bytes
				item->value.lng.high = be32toh(item->value.lng.high);
				item->value.lng.low = be32toh(item->value.lng.low);
				// 8-byte consts take 2 pool entries
				++i;
				table_size_bytes += 8;
				break;
			case DOUBLE: // Double: a 64-bit double-precision IEEE 754 floating-point number (takes two slots in the constant pool table)
				opcode_memcpy(&item->value.dbl.high,bytecode, sizeof(item->value.dbl.high)); // 4 bytes
				opcode_memcpy(&item->value.dbl.low,bytecode, sizeof(item->value.dbl.low)); // 4 bytes
				item->value.dbl.high = be32toh(item->value.dbl.high);
				item->value.dbl.low = be32toh(item->value.dbl.low);
				// 8-byte consts take 2 pool entries
				++i;
				table_size_bytes += 8;
				break;
			case CLASS: // Class reference: an uint16 within the constant pool to a UTF-8 string containing the fully qualified class name
				opcode_memcpy(&r.class_idx,bytecode, sizeof(r.class_idx));
				r.class_idx = be16toh(r.class_idx);
				item->value.ref = r;
				table_size_bytes += 2;
				break;
			case STRING: // String reference: an uint16 within the constant pool to a UTF-8 string
				opcode_memcpy(&r.class_idx,bytecode, sizeof(r.class_idx));
				r.class_idx = be16toh(r.class_idx);
				item->value.ref = r;
				table_size_bytes += 2;
				break;
			case FIELD: // Field reference: two uint16 within the pool, 1st pointing to a Class reference, 2nd to a Name and Type descriptor
				/* FALL THROUGH TO METHOD */
			case METHOD: // Method reference: two uint16s within the pool, 1st pointing to a Class reference, 2nd to a Name and Type descriptor
				/* FALL THROUGH TO INTERFACE_METHOD */
			case INTERFACE_METHOD: // Interface method reference: 2 uint16 within the pool, 1st pointing to a Class reference, 2nd to a Name and Type descriptor
				opcode_memcpy(&r.class_idx,bytecode, sizeof(r.class_idx));
				opcode_memcpy(&r.name_idx,bytecode, sizeof(r.name_idx));
				r.class_idx = be16toh(r.class_idx);
				r.name_idx = be16toh(r.name_idx);
				item->value.ref = r;
				table_size_bytes += 4;
				break;
			case NAME: // Name and type descriptor: 2 uint16 to UTF-8 strings, 1st representing a name (identifier), 2nd a specially encoded type descriptor
				opcode_memcpy(&r.class_idx,bytecode, sizeof(r.class_idx));
				opcode_memcpy(&r.name_idx,bytecode, sizeof(r.name_idx));
				r.class_idx = be16toh(r.class_idx);
				r.name_idx = be16toh(r.name_idx);
				item->value.ref = r;
				table_size_bytes += 4;
				break;
			default:
				fprintf(stderr, "Found tag byte '%d' but don't know what to do with it\n", tag_byte);
				item = NULL;
				break;
		}
		if (item != NULL) class->items[i-1] = *item;
	}
	class->pool_size_bytes = table_size_bytes;
}

bool is_class(Bytecode *bytecode) {
	uint32_t magicNum;
	opcode_memcpy(&magicNum,bytecode, sizeof(uint32_t));
	return be32toh(magicNum) == 0xcafebabe;
}

Item *get_item(const Class *class, const uint16_t cp_idx) {
	if (cp_idx < class->const_pool_count) return &class->items[cp_idx-1];
	else return NULL;
}

Item *get_class_string(const Class *class, const uint16_t index) {
	Item *i1 = get_item(class, index);
	return get_item(class, i1->value.ref.class_idx);
}

double to_double(const Double dbl) {
	return -dbl.high; //FIXME check the following implementation
	//unsigned long bits = ((long) be32toh(item->dbl.high) << 32) + be32toh(item->dbl.low);
	//if (bits == 0x7ff0000000000000L) {
		//return POSITIVE_INFINITY;
	//} else if (bits == 0xfff0000000000000L) {
		//return POSITIVE_INFINITY;
	//} else if ((bits > 0x7ff0000000000000L && bits < 0x7fffffffffffffffL) 
			//|| (bits > 0xfff0000000000001L && bits < 0xffffffffffffffffL)) {
		//return NaN;
	//} else {
		//int s = ((bits >> 63) == 0) ? 1 : -1; 
		//int e = (int)((bits >> 52) & 0x7ffL);
		//long m = (e == 0) 
			//? (bits & 0xfffffffffffffL) << 1 
			//: (bits & 0xfffffffffffffL) | 0x10000000000000L;
		//return s * m * (2 << (e - 1075)); //FIXME this is wrong
	//} 
}

long to_long(const Long lng) {
	return ((long) be32toh(lng.high) << 32) + be32toh(lng.low);
}
void opcode_memcpy(void *target,Bytecode *bytecode,size_t len){
	memcpy(target,bytecode.data + bytecode->index,len);
	bytecode->index += len;
}

char *field2str(const char fld_type) {
	switch(fld_type) {
		case 'B': 
			return "byte";
		case 'C': 
			return "char";
		case 'D': 
			return "double";
		case 'F': 
			return "float";
		case 'I': 
			return "int";
		case 'J': 
			return "long";
		case 'L': 
			return "reference";
		case 'S': 
			return "short";
		case 'Z': 
			return "boolean";
		case '[': 
			return "array";
		default:
			return "Undefined";
	}
}

