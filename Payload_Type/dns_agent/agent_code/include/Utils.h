#ifndef UTILS_H
#define UTILS_H

#include <windows.h>
#include <stddef.h>   // size_t

// Base64 encoding/decoding
char* base64_encode(const unsigned char* data, size_t input_length, size_t* output_length);
unsigned char* base64_decode(const char* data, size_t input_length, size_t* output_length);

// UUID generation
char* generate_uuid();

// Time functions
char* get_timestamp();
void sleep_ms(DWORD milliseconds);

// String manipulation
char** str_split(const char* str, char delim, int* count);
void free_tokens(char** tokens, int count);
char* url_encode(const char* str);
void str_tolower(char* str);
void str_toupper(char* str);
BOOL str_startswith(const char* str, const char* prefix);
BOOL str_endswith(const char* str, const char* suffix);
char* str_trim(const char* str);

// File operations
BOOL file_exists(const char* filename);
BOOL directory_exists(const char* dirname);
BOOL create_directory(const char* dirname);
BOOL copy_file(const char* source, const char* destination, BOOL overwrite);
BOOL move_file(const char* source, const char* destination);
BOOL delete_file(const char* filename);
long get_file_size(const char* filename);
unsigned char* read_file(const char* filename, size_t* size);
BOOL write_file(const char* filename, const unsigned char* data, size_t size);
BOOL append_to_file(const char* filename, const unsigned char* data, size_t size);

// System information
char* get_hostname();
char* get_username();
char* get_current_directory();

// Random number generation
int get_random_int(int min, int max);
DWORD apply_jitter(DWORD time_ms, DWORD jitter_percent);

// Encryption
void xor_encrypt(unsigned char* data, size_t data_len, const unsigned char* key, size_t key_len);
void xor_decrypt(unsigned char* data, size_t data_len, const unsigned char* key, size_t key_len);

#endif // UTILS_H
