#pragma once

#include <stddef.h>

#ifndef nullptr // vscode shananigans
#define nullptr NULL
#endif

void* memcpy( void* dest, const void* src, size_t n );
void* memmove( void* dest, const void* src, size_t n );
char* strcpy( char* dest, const char* src );
char* strncpy( char* dest, const char* src, size_t n );
char* strcat( char* dest, const char* src );
char* strncat( char* dest, const char* src, size_t n );
int memcmp( const void* s1, const void* s2, size_t n );
int strcmp( const char* s1, const char* s2 );
int strncmp( const char* s1, const char* s2, size_t n );
void* memchr( const void* s, int c, size_t n );
char* strchr( const char* s, int c );
char* strrchr( const char* s, int c );
char* strdup( const char* s );
size_t strlen( const char* s );
size_t strnlen( const char* s, size_t maxlen );
void* memset( void* s, int c, size_t n );
size_t strspn( const char* s, const char* accept );
size_t strcspn( const char* s, const char* reject );
char* strpbrk( const char* s, const char* accept );
char* strstr( const char* haystack, const char* needle );
char* strtok( char* str, const char* delim );
int isalnum( int c );
int isalpha( int c );
int iscntrl( int c );
int isdigit( int c );
int isgraph( int c );
int islower( int c );
int isprint( int c );
int ispunct( int c );
int isspace( int c );
int isupper( int c );
int isxdigit( int c );
int tolower( int c );
int toupper( int c );
int atoi( const char* str );
long atol( const char* str );
long long atoll( const char* str );
char* itoa( int num, char* str, int base );
char* ltoa( long num, char* str, int base );
char* lltoa( long long num, char* str, int base );
long strtol( const char* str, char** endptr, int base );
long long strtoll( const char* str, char** endptr, int base );
