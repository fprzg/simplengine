#pragma once

#include <stdarg.h>
#include <ctype.h>

#include "simplengine.h"

b8
SkipSpace(char **StrPtr)
{
    char *Str = *StrPtr;
    
    while(isspace((unsigned char)*Str))
    {
        ++Str;
    }
    
    b8 Moved = Str - *StrPtr;
    *StrPtr = Str;
    
    return Moved;
}

u32
SScanf(char **StrPtr, char *FmtParam, ...)
{
    va_list Args;
    u32 Count = 0;
    char *Str = *StrPtr;
    char *Fmt = FmtParam;
    
    va_start(Args, FmtParam);
    
    while(*Str && *Fmt)
    {
        SkipSpace(&Fmt);
        SkipSpace(&Str);
        
        if(*Fmt == *Str)
        {
            ++Fmt;
            ++Str;
        }
        else if(*Fmt == '%')
        {
            ++Fmt;
            
            switch(*Fmt)
            {
                case 'd':
                {
                    SkipSpace(&Str);
                    
                    int *Value = va_arg(Args, int*);
                    int Number = 0;
                    b8 Negative = false;
                    
                    if(*Str == '-')
                    {
                        Negative = true;
                        ++Str;
                    }
                    else if(*Str == '+')
                    {
                        ++Str;
                    }
                    
                    if(!isdigit((unsigned char)*Str))
                    {
                        goto End;
                    }
                    
                    while(isdigit((unsigned char)*Str))
                    {
                        Number = Number * 10 + (*Str - '0');
                        ++Str;
                    }
                    
                    *Value = Negative ? -Number : Number;
                    ++Count;
                    
                } break;
                
                case 'f':
                {
                    SkipSpace(&Str);
                    
                    float *Value = va_arg(Args, float *);
                    float Number = 0.0f;
                    float Fraction = 0.1f;
                    b8 Negative = false;
                    
                    if(*Str == '-')
                    {
                        Negative = true;
                        ++Str;
                    }
                    else if(*Str == '+')
                    {
                        ++Str;
                    }
                    
                    if(!isdigit((unsigned char)*Str) && *Str != '.')
                    {
                        goto End;
                    }
                    
                    while(isdigit((unsigned char)*Str))
                    {
                        Number = Number * 10.0f + (float)(*Str - '0');
                        ++Str;
                    }
                    
                    if(*Str == '.')
                    {
                        ++Str;
                        
                        while(isdigit((unsigned char)*Str))
                        {
                            Number += (float)(*Str - '0') * Fraction;
                            Fraction *= 0.1f;
                            ++Str;
                        }
                    }
                    
                    
                    // Handle cientific notation
                    if(*Str == 'e' || *Str == 'E')
                    {
                        ++Str;
                        int Exp = 0;
                        b8 ExpNegative = false;
                        if(*Str == '-')
                        {
                            ++Str;
                            ExpNegative = true;
                        }
                        else if(*Str == '+')
                        {
                            ++Str;
                        }
                        
                        while(isdigit((unsigned char)*Str))
                        {
                            Exp = Exp * 10 + (*Str - '0');
                            ++Str;
                        }
                        
                        Exp = ExpNegative ? -Exp : Exp;
                        while(Exp > 0)
                        {
                            Number *= 10.0f;
                            Exp--;
                        }
                        
                        while(Exp < 0)
                        {
                            Number *= 0.1f;
                            Exp++;
                        }
                    }
                    
                    *Value = Negative ? -Number : Number;
                    ++Count;
                } break;
                
                case 's':
                {
                    char *Value = va_arg(Args, char *);
                    int I = 0;
                    
                    SkipSpace(&Str);
                    while(Str != '\0' && !isspace((unsigned char)*Str))
                    {
                        Value[I] = *Str;
                        ++Str;
                        ++I;
                    }
                    
                    Value[I] = '\0';
                    ++Count;
                } break;
                
                default:
                {
                    goto End;
                }
            }
            
            ++Fmt;
        }
        else
        {
            break;
        }
    }
    
    
    End:
    va_end(Args);
    *StrPtr = Str;
    
    return Count;
}