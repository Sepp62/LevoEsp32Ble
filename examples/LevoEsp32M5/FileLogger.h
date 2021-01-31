#ifndef FILE_LOGGER_H
#define FILE_LOGGER_H

class FileLogger
{
public:
    bool   Writeln( const char* strLog );
    bool   Open();
    void   Close();
    int8_t PercentFull();
};

#endif // FILE_LOGGER_H