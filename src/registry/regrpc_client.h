/*
 *    
 *    Copyright (C) 2006, Network Resonance, Inc.
 *    All Rights Reserved
 *    
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions
 *    are met:
 *    
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of Network Resonance, Inc. nor the name of any
 *       contributors to this software may be used to endorse or promote 
 *       products derived from this software without specific prior written
 *       permission.
 *    
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 *    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *    POSSIBILITY OF SUCH DAMAGE.
 *    
 */

static int nr_reg_get_char_1_clnt(NR_registry name, char *data);
static int nr_reg_get_uchar_1_clnt(NR_registry name, UCHAR *data);
static int nr_reg_get_int2_1_clnt(NR_registry name, INT2 *data);
static int nr_reg_get_uint2_1_clnt(NR_registry name, UINT2 *data);
static int nr_reg_get_int4_1_clnt(NR_registry name, INT4 *data);
static int nr_reg_get_uint4_1_clnt(NR_registry name, UINT4 *data);
static int nr_reg_get_int8_1_clnt(NR_registry name, INT8 *data);
static int nr_reg_get_uint8_1_clnt(NR_registry name, UINT8 *data);
static int nr_reg_get_double_1_clnt(NR_registry name, double *data);
static int nr_reg_get_registry_1_clnt(NR_registry name, NR_registry *data);
static int nr_reg_get_bytes_1_clnt(NR_registry name, UCHAR *data, size_t size, size_t *length);
static int nr_reg_get_string_1_clnt(NR_registry name, char *data, size_t size);
static int nr_reg_get_length_1_clnt(NR_registry name, size_t *len);
static int nr_reg_set_char_1_clnt(NR_registry name, char data);
static int nr_reg_set_uchar_1_clnt(NR_registry name, UCHAR data);
static int nr_reg_set_int2_1_clnt(NR_registry name, INT2 data);
static int nr_reg_set_uint2_1_clnt(NR_registry name, UINT2 data);
static int nr_reg_set_int4_1_clnt(NR_registry name, INT4 data);
static int nr_reg_set_uint4_1_clnt(NR_registry name, UINT4 data);
static int nr_reg_set_int8_1_clnt(NR_registry name, INT8 data);
static int nr_reg_set_uint8_1_clnt(NR_registry name, UINT8 data);
static int nr_reg_set_double_1_clnt(NR_registry name, double data);
static int nr_reg_set_registry_1_clnt(NR_registry name);
static int nr_reg_set_bytes_1_clnt(NR_registry name, UCHAR *data, size_t length);
static int nr_reg_set_string_1_clnt(NR_registry name, char *data);
static int nr_reg_del_1_clnt(NR_registry name);
static int nr_reg_get_child_count_1_clnt(NR_registry parent, size_t *count);
static int nr_reg_get_children_1_clnt(NR_registry parent, NR_registry *data, size_t size, size_t *length);
static int nr_reg_dump_1_clnt(int sorted);

