#ifndef __RECORDS_H_INCLUDED__
#define __RECORDS_H_INCLUDED__

struct Record
{
    char* name;
    char* num;
};

struct Records
{
    struct Record record;
    struct Records* next;
};

#endif
