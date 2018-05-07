#include "class.h"

void print_class(const Class *class) {

    printf("Minor number: %u \n",
    class->minor_version);
    printf("Major number: %u \n",
    class->major_version);
    printf("Constant pool size: %u \n",
    class->const_pool_count);
    printf("Constant table size: %ub \n",
    class->pool_size_bytes);
    printf("Printing constant pool of %d items...\n",
    class->const_pool_count - 1);

    Item *s;
    uint16_t i = 1; // constant pool indexes start at 1, get_item converts to pointer index
    while (i < class->const_pool_count) {
        s = get_item(
        class, i);
        printf("Item #%u %s: ", i, tag2str(s->tag));
        if (s->tag == STRING_UTF8) {
            printf("%s\n", s->value.string.value);
        } else if (s->tag == INTEGER) {
            printf("%d\n", s->value.integer);
        } else if (s->tag == FLOAT) {
            printf("%f\n", s->value.flt);
        } else if (s->tag == LONG) {
            printf("%ld\n", to_long(s->value.lng));
        } else if (s->tag == DOUBLE) {
            printf("%lf\n", to_double(s->value.dbl));
        } else if (s->tag == CLASS || s->tag == STRING) {
            printf("%u\n", s->value.ref.class_idx);
        } else if (s->tag == FIELD || s->tag == METHOD || s->tag == INTERFACE_METHOD || s->tag == NAME) {
            printf("%u.%u\n", s->value.ref.class_idx, s->value.ref.name_idx);
        }
        i++;
    }

    printf("Access flags: %x\n",
    class->flags);

    Item * cl_str = get_class_string(
    class, class->this_class);
    printf("This class: %s\n", cl_str->value.string.value);

    cl_str = get_class_string(
    class, class->super_class);
    printf("Super class: %s\n", cl_str->value.string.value);

    printf("Interfaces count: %u\n",
    class->interfaces_count);

    printf("Printing %u interfaces...\n",
    class->interfaces_count);
    if (class->interfaces_count > 0) {
        Ref * iface =
        class->interfaces;
        Item *the_class;
        uint16_t idx = 0;
        while (idx < class->interfaces_count) {
            the_class = get_item(
            class, iface->class_idx); // the interface class reference
            Item * item = get_item(
            class, the_class->value.ref.class_idx);
            String string = item->value.string;
            printf("Interface: %s\n", string.value);
            idx++;
            iface =
            class->interfaces + idx; // next Ref
        }
    }

    printf("Printing %d fields...\n",
    class->fields_count);

    if (class->fields_count > 0) {
        Field * field =
        class->fields;
        uint16_t idx = 0;
        while (idx < class->fields_count) {
            Item * name = get_item(
            class, field->name_idx);
            Item * desc = get_item(
            class, field->desc_idx);
            printf("%s %s\n", field2str(desc->value.string.value[0]), name->value.string.value);
            Attribute at;
            if (field->attrs_count > 0) {
                int aidx = 0;
                while (aidx < field->attrs_count) {
                    at = field->attrs[aidx];
                    Item * name = get_item(
                    class, at.name_idx);
                    printf("\tAttribute name: %s\n", name->value.string.value);
                    printf("\tAttribute length %d\n", at.length);
                    printf("\tAttribute: %s\n", at.info);
                    aidx++;
                }
            }
            idx++;
            field =
            class->fields + idx;
        }
    }

    printf("Printing %u methods...\n",
    class->methods_count);
    i = 0;
    if (class->methods_count > 0) {
        Method * method =
        class->methods;
        uint16_t idx = 0;
        while (idx < class->methods_count) {
            Item * name = get_item(
            class, method->name_idx);
            Item * desc = get_item(
            class, method->desc_idx);
            printf("%s %s\n", name->value.string.value, desc->value.string.value);
            Attribute at;
            if (method->attrs_count > 0) {
                int aidx = 0;
                while (aidx < method->attrs_count) {
                    at = method->attrs[aidx];
                    Item * name = get_item(
                    class, at.name_idx);
                    printf("\tAttribute name: %s", name->value.string.value);
                    printf("\tAttribute length %d\n", at.length);
                    printf("\tAttribute: %s\n", at.info);
                    aidx++;
                }
            }
            idx++;
            method =
            class->methods + idx;
        }
    }

    printf("Printing %u attributes...\n",
    class->attributes_count);
    if (class->attributes_count > 0) {
        Attribute at;
        int aidx = 0;
        while (aidx < class->attributes_count) {
            at =
            class->attributes[aidx];
            Item * name = get_item(
            class, at.name_idx);
            printf("\tAttribute name: %s", name->value.string.value);
            printf("\tAttribute length %d\n", at.length);
            printf("\tAttribute: %s\n", at.info);
            aidx++;
        }
    }
}

