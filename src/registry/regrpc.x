/**
   regrpc.x

   Copyright (C) 2006, Network Resonance, Inc.
   All Rights Reserved.

   briank@network-resonance.com Tue Jan 31 18:39:00 PST 2006
*/

typedef string                 nr_regd_registry<>;
typedef opaque                 nr_regd_data<>;
typedef string                 nr_regd_string<>;

struct nr_regd_result_char {
    int              retval;
    char             out;
};

struct nr_regd_result_UCHAR {
    int              retval;
    u_char           out;
};

struct nr_regd_result_INT2 {
    int              retval;
    int16_t          out;
};

struct nr_regd_result_UINT2 {
    int              retval;
    u_int16_t        out;
};

struct nr_regd_result_INT4 {
    int              retval;
    int32_t          out;
};

struct nr_regd_result_UINT4 {
    int              retval;
    u_int32_t        out;
};

struct nr_regd_result_INT8 {
    int              retval;
    int64_t          out;
};

struct nr_regd_result_UINT8 {
    int              retval;
    u_int64_t        out;
};

struct nr_regd_result_double {
    int              retval;
    double           out;
};

struct nr_regd_result_bytes {
    int              retval;
    opaque           out<>;
};

struct nr_regd_result_string {
    int              retval;
    string           out<>;
};

struct nr_regd_result_strings {
    int              retval;
    nr_regd_string   out<>;
};

program REGISTRYDPROG {
    version REGISTRYDVERS {

        nr_regd_result_char       NR_REG_GET_CHAR(nr_regd_registry name) = 1;
        nr_regd_result_UCHAR      NR_REG_GET_UCHAR(nr_regd_registry name) = 2;
        nr_regd_result_INT2       NR_REG_GET_INT2(nr_regd_registry name) = 3;
        nr_regd_result_UINT2      NR_REG_GET_UINT2(nr_regd_registry name) = 4;
        nr_regd_result_INT4       NR_REG_GET_INT4(nr_regd_registry name) = 5;
        nr_regd_result_UINT4      NR_REG_GET_UINT4(nr_regd_registry name) = 6;
        nr_regd_result_INT8       NR_REG_GET_INT8(nr_regd_registry name) = 7;
        nr_regd_result_UINT8      NR_REG_GET_UINT8(nr_regd_registry name) = 8;
        nr_regd_result_double     NR_REG_GET_DOUBLE(nr_regd_registry name) = 9;
        nr_regd_result_string     NR_REG_GET_REGISTRY(nr_regd_registry name) = 10;

        nr_regd_result_bytes      NR_REG_GET_BYTES(nr_regd_registry name) = 11;
        nr_regd_result_string     NR_REG_GET_STRING(nr_regd_registry name) = 12;

        nr_regd_result_UINT4      NR_REG_GET_LENGTH(nr_regd_registry name) = 13;
        nr_regd_result_string     NR_REG_GET_TYPE(nr_regd_registry name) = 14;

        int     NR_REG_SET_CHAR(nr_regd_registry name, char data) = 15;
        int     NR_REG_SET_UCHAR(nr_regd_registry name, u_char data) = 16;
        int     NR_REG_SET_INT2(nr_regd_registry name, int16_t data) = 17;
        int     NR_REG_SET_UINT2(nr_regd_registry name, u_int16_t data) = 18;
        int     NR_REG_SET_INT4(nr_regd_registry name, int32_t data) = 19;
        int     NR_REG_SET_UINT4(nr_regd_registry name, u_int32_t data) = 20;
        int     NR_REG_SET_INT8(nr_regd_registry name, int64_t data) = 21;
        int     NR_REG_SET_UINT8(nr_regd_registry name, u_int64_t data) = 22;
        int     NR_REG_SET_DOUBLE(nr_regd_registry name, double data) = 23;
        int     NR_REG_SET_REGISTRY(nr_regd_registry name) = 24;
    
        int     NR_REG_SET_BYTES(nr_regd_registry name, nr_regd_data data) = 25;
        int     NR_REG_SET_STRING(nr_regd_registry name, nr_regd_string data) = 26; 

        int     NR_REG_DEL(nr_regd_registry name) = 27;

        int     NR_REG_FIN(nr_regd_registry name) = 28;

        nr_regd_result_UINT4    NR_REG_GET_CHILD_COUNT(nr_regd_registry parent) = 29;
        nr_regd_result_strings  NR_REG_GET_CHILDREN(nr_regd_registry parent) = 30;

        int     NR_REG_DUMP() = 31;

        int     NR_REG_REGISTER_FOR_CALLBACKS(int port) = 32;

    } = 1;
} = 0x20000189; 

