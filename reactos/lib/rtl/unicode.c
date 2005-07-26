/*
 * Rtl string functions
 *
 * Copyright (C) 1996-1998 Marcus Meissner
 * Copyright (C) 2000      Alexandre Julliard
 * Copyright (C) 2003      Thomas Mertes
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#define __NTDRIVER__
#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern BOOLEAN NlsMbCodePageTag;
extern BOOLEAN NlsMbOemCodePageTag;

/* FUNCTIONS *****************************************************************/


/*
* @implemented
*/
WCHAR STDCALL
RtlAnsiCharToUnicodeChar (IN CHAR AnsiChar)
{
   ULONG Size;
   WCHAR UnicodeChar;

   Size = 1;
#if 0

   Size = (NlsLeadByteInfo[AnsiChar] == 0) ? 1 : 2;
#endif

   RtlMultiByteToUnicodeN (&UnicodeChar,
                           sizeof(WCHAR),
                           NULL,
                           &AnsiChar,
                           Size);

   return UnicodeChar;
}


/*
 * @implemented
 *
 * RETURNS
 *  The calculated size in bytes including nullterm.
 */
ULONG
STDCALL
RtlAnsiStringToUnicodeSize(IN PANSI_STRING AnsiString)
{
   ULONG Size;

   RtlMultiByteToUnicodeSize(&Size,
                             AnsiString->Buffer,
                             AnsiString->Length);

   return(Size);
}



/*
 * @implemented
 *
 * NOTES
 *  If src->length is zero dest is unchanged.
 *  Dest is never nullterminated.
 */
NTSTATUS
STDCALL
RtlAppendStringToString(IN OUT PSTRING Destination,
                        IN PSTRING Source)
{
   PCHAR Ptr;

   if (Source->Length == 0)
      return(STATUS_SUCCESS);

   if (Destination->Length + Source->Length >= Destination->MaximumLength)
      return(STATUS_BUFFER_TOO_SMALL);

   Ptr = Destination->Buffer + Destination->Length;
   memmove(Ptr,
           Source->Buffer,
           Source->Length);
   Ptr += Source->Length;
   *Ptr = 0;

   Destination->Length += Source->Length;

   return(STATUS_SUCCESS);
}



/*
 * @implemented
 *
 * NOTES
 *  If src->length is zero dest is unchanged.
 *  Dest is nullterminated when the MaximumLength allowes it.
 *  When dest fits exactly in MaximumLength characters the nullterm is ommitted.
 */
NTSTATUS
STDCALL
RtlAppendUnicodeStringToString(
   IN OUT PUNICODE_STRING Destination,
   IN PUNICODE_STRING Source)
{

   if ((Source->Length + Destination->Length) > Destination->MaximumLength)
      return STATUS_BUFFER_TOO_SMALL;

   memcpy((char*)Destination->Buffer + Destination->Length, Source->Buffer, Source->Length);
   Destination->Length += Source->Length;
   /* append terminating '\0' if enough space */
   if( Destination->MaximumLength > Destination->Length )
      Destination->Buffer[Destination->Length / sizeof(WCHAR)] = 0;

   return STATUS_SUCCESS;
}


/**************************************************************************
 *      RtlCharToInteger   (NTDLL.@)
 * @implemented
 * Converts a character string into its integer equivalent.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. value contains the converted number
 *  Failure: STATUS_INVALID_PARAMETER, if base is not 0, 2, 8, 10 or 16.
 *           STATUS_ACCESS_VIOLATION, if value is NULL.
 *
 * NOTES
 *  For base 0 it uses 10 as base and the string should be in the format
 *      "{whitespace} [+|-] [0[x|o|b]] {digits}".
 *  For other bases the string should be in the format
 *      "{whitespace} [+|-] {digits}".
 *  No check is made for value overflow, only the lower 32 bits are assigned.
 *  If str is NULL it crashes, as the native function does.
 *
 * DIFFERENCES
 *  This function does not read garbage behind '\0' as the native version does.
 */
NTSTATUS
STDCALL
RtlCharToInteger(
    PCSZ str,      /* [I] '\0' terminated single-byte string containing a number */
    ULONG base,    /* [I] Number base for conversion (allowed 0, 2, 8, 10 or 16) */
    PULONG value)  /* [O] Destination for the converted value */
{
    CHAR chCurrent;
    int digit;
    ULONG RunningTotal = 0;
    char bMinus = 0;

    while (*str != '\0' && *str <= ' ') {
	str++;
    } /* while */

    if (*str == '+') {
	str++;
    } else if (*str == '-') {
	bMinus = 1;
	str++;
    } /* if */

    if (base == 0) {
	base = 10;
	if (str[0] == '0') {
	    if (str[1] == 'b') {
		str += 2;
		base = 2;
	    } else if (str[1] == 'o') {
		str += 2;
		base = 8;
	    } else if (str[1] == 'x') {
		str += 2;
		base = 16;
	    } /* if */
	} /* if */
    } else if (base != 2 && base != 8 && base != 10 && base != 16) {
	return STATUS_INVALID_PARAMETER;
    } /* if */

    if (value == NULL) {
	return STATUS_ACCESS_VIOLATION;
    } /* if */

    while (*str != '\0') {
	chCurrent = *str;
	if (chCurrent >= '0' && chCurrent <= '9') {
	    digit = chCurrent - '0';
	} else if (chCurrent >= 'A' && chCurrent <= 'Z') {
	    digit = chCurrent - 'A' + 10;
	} else if (chCurrent >= 'a' && chCurrent <= 'z') {
	    digit = chCurrent - 'a' + 10;
	} else {
	    digit = -1;
	} /* if */
	if (digit < 0 || digit >= (int)base) {
	    *value = bMinus ? -RunningTotal : RunningTotal;
	    return STATUS_SUCCESS;
	} /* if */

	RunningTotal = RunningTotal * base + digit;
	str++;
    } /* while */

    *value = bMinus ? -RunningTotal : RunningTotal;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
LONG
STDCALL
RtlCompareString(
   IN PSTRING String1,
   IN PSTRING String2,
   IN BOOLEAN CaseInsensitive)
{
   ULONG len1, len2;
   PCHAR s1, s2;
   CHAR  c1, c2;

   if (String1 && String2)
   {
      len1 = String1->Length;
      len2 = String2->Length;
      s1 = String1->Buffer;
      s2 = String2->Buffer;

      if (s1 && s2)
      {
         if (CaseInsensitive)
         {
            for(;;)
            {
               c1 = len1-- ? RtlUpperChar (*s1++) : 0;
               c2 = len2-- ? RtlUpperChar (*s2++) : 0;
               if (!c1 || !c2 || c1 != c2)
                  return c1 - c2;
            }
         }
         else
         {
            for(;;)
            {
               c1 = len1-- ? *s1++ : 0;
               c2 = len2-- ? *s2++ : 0;
               if (!c1 || !c2 || c1 != c2)
                  return c1 - c2;
            }
         }
      }
   }

   return 0;
}


/*
 * @implemented
 *
 * RETURNS
 *  TRUE if strings are equal.
 */
BOOLEAN
STDCALL
RtlEqualString(
   IN PSTRING String1,
   IN PSTRING String2,
   IN BOOLEAN CaseInsensitive)
{
   ULONG i;
   CHAR c1, c2;
   PCHAR p1, p2;

   if (String1->Length != String2->Length)
      return FALSE;

   p1 = String1->Buffer;
   p2 = String2->Buffer;
   for (i = 0; i < String1->Length; i++)
   {
      if (CaseInsensitive == TRUE)
      {
         c1 = RtlUpperChar (*p1);
         c2 = RtlUpperChar (*p2);
      }
      else
      {
         c1 = *p1;
         c2 = *p2;
      }

      if (c1 != c2)
         return FALSE;

      p1++;
      p2++;
   }

   return TRUE;
}


/*
 * @implemented
 *
 * RETURNS
 *  TRUE if strings are equal.
 */
BOOLEAN
STDCALL
RtlEqualUnicodeString(
   IN CONST UNICODE_STRING *String1,
   IN CONST UNICODE_STRING *String2,
   IN BOOLEAN  CaseInsensitive)
{
   ULONG i;
   WCHAR wc1, wc2;
   PWCHAR pw1, pw2;

   if (String1->Length != String2->Length)
      return FALSE;

   pw1 = String1->Buffer;
   pw2 = String2->Buffer;

   for (i = 0; i < String1->Length / sizeof(WCHAR); i++)
   {
      if (CaseInsensitive == TRUE)
      {
         wc1 = RtlUpcaseUnicodeChar (*pw1);
         wc2 = RtlUpcaseUnicodeChar (*pw2);
      }
      else
      {
         wc1 = *pw1;
         wc2 = *pw2;
      }

      if (wc1 != wc2)
         return FALSE;

      pw1++;
      pw2++;
   }

   return TRUE;
}


/*
 * @implemented
 */
VOID
STDCALL
RtlFreeAnsiString(IN PANSI_STRING AnsiString)
{
   if (AnsiString->Buffer != NULL)
   {
      RtlpFreeStringMemory(AnsiString->Buffer, TAG_ASTR);

      AnsiString->Buffer = NULL;
      AnsiString->Length = 0;
      AnsiString->MaximumLength = 0;
   }
}


/*
 * @implemented
 */
VOID
STDCALL
RtlFreeOemString(IN POEM_STRING OemString)
{
   if (OemString->Buffer != NULL)
   {
      RtlpFreeStringMemory(OemString->Buffer, TAG_OSTR);

      OemString->Buffer = NULL;
      OemString->Length = 0;
      OemString->MaximumLength = 0;
   }
}


/*
 * @implemented
 */
VOID
STDCALL
RtlFreeUnicodeString(IN PUNICODE_STRING UnicodeString)
{
   if (UnicodeString->Buffer != NULL)
   {
      RtlpFreeStringMemory(UnicodeString->Buffer, TAG_USTR);

      UnicodeString->Buffer = NULL;
      UnicodeString->Length = 0;
      UnicodeString->MaximumLength = 0;
   }
}

/*
* @unimplemented
*/
BOOLEAN
STDCALL
RtlIsValidOemCharacter (
	IN PWCHAR Char
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @implemented
 *
 * NOTES
 *  If source is NULL the length of source is assumed to be 0.
 */
VOID
STDCALL
RtlInitAnsiString(IN OUT PANSI_STRING DestinationString,
                  IN PCSZ SourceString)
{
   ULONG DestSize;

   if (SourceString == NULL)
   {
      DestinationString->Length = 0;
      DestinationString->MaximumLength = 0;
   }
   else
   {
      DestSize = strlen ((const char *)SourceString);
      DestinationString->Length = DestSize;
      DestinationString->MaximumLength = DestSize + sizeof(CHAR);
   }
   DestinationString->Buffer = (PCHAR)SourceString;
}



/*
 * @implemented
 *
 * NOTES
 *  If source is NULL the length of source is assumed to be 0.
 */
VOID
STDCALL
RtlInitString(
   IN OUT PSTRING DestinationString,
   IN PCSZ SourceString)
{
   ULONG DestSize;

   if (SourceString == NULL)
   {
      DestinationString->Length = 0;
      DestinationString->MaximumLength = 0;
   }
   else
   {
      DestSize = strlen((const char *)SourceString);
      DestinationString->Length = DestSize;
      DestinationString->MaximumLength = DestSize + sizeof(CHAR);
   }
   DestinationString->Buffer = (PCHAR)SourceString;
}


/*
 * @implemented
 *
 * NOTES
 *  If source is NULL the length of source is assumed to be 0.
 */
VOID
STDCALL
RtlInitUnicodeString(IN OUT PUNICODE_STRING DestinationString,
                     IN PCWSTR SourceString)
{
   ULONG DestSize;

   DPRINT("RtlInitUnicodeString(DestinationString 0x%p, SourceString 0x%p)\n",
          DestinationString,
          SourceString);

   if (SourceString == NULL)
   {
      DestinationString->Length = 0;
      DestinationString->MaximumLength = 0;
   }
   else
   {
      DestSize = wcslen((PWSTR)SourceString) * sizeof(WCHAR);
      DestinationString->Length = DestSize;
      DestinationString->MaximumLength = DestSize + sizeof(WCHAR);
   }
   DestinationString->Buffer = (PWSTR)SourceString;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
RtlInitUnicodeStringEx(OUT PUNICODE_STRING DestinationString,
                       IN PCWSTR SourceString)
{
   ULONG Length;

   if (SourceString != NULL)
   {
      Length = wcslen(SourceString) * sizeof(WCHAR);
      if (Length > 0xFFFC)
         return STATUS_NAME_TOO_LONG;

      DestinationString->Length = Length;
      DestinationString->MaximumLength = Length + sizeof(WCHAR);
      DestinationString->Buffer = (PWSTR)SourceString;
   }
   else
   {
      DestinationString->Length = 0;
      DestinationString->MaximumLength = 0;
      DestinationString->Buffer = NULL;
   }

   return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * NOTES
 *  Writes at most length characters to the string str.
 *  Str is nullterminated when length allowes it.
 *  When str fits exactly in length characters the nullterm is ommitted.
 */
NTSTATUS
STDCALL
RtlIntegerToChar(
   IN ULONG Value,
   IN ULONG Base,
   IN ULONG Length,
   IN OUT PCHAR String)
{
   ULONG Radix;
   CHAR  temp[33];
   ULONG v = Value;
   ULONG i;
   PCHAR tp;
   PCHAR sp;

   Radix = Base;
   if (Radix == 0)
      Radix = 10;

   if ((Radix != 2) && (Radix != 8) &&
       (Radix != 10) && (Radix != 16))
   {
      return STATUS_INVALID_PARAMETER;
   }

   tp = temp;
   while (v || tp == temp)
   {
      i = v % Radix;
      v = v / Radix;
      if (i < 10)
         *tp = i + '0';
      else
         *tp = i + 'a' - 10;
      tp++;
   }

   if ((ULONG)((ULONG_PTR)tp - (ULONG_PTR)temp) >= Length)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }

   sp = String;
   while (tp > temp)
      *sp++ = *--tp;
   *sp = 0;

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlIntegerToUnicode(
    IN ULONG Value,
    IN ULONG Base  OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN OUT LPWSTR String
    )
{
   ULONG Radix;
   WCHAR  temp[33];
   ULONG v = Value;
   ULONG i;
   PWCHAR tp;
   PWCHAR sp;

   Radix = Base;
   if (Radix == 0)
      Radix = 10;

   if ((Radix != 2) && (Radix != 8) &&
       (Radix != 10) && (Radix != 16))
   {
      return STATUS_INVALID_PARAMETER;
   }

   tp = temp;
   while (v || tp == temp)
   {
      i = v % Radix;
      v = v / Radix;
      if (i < 10)
         *tp = i + L'0';
      else
         *tp = i + L'a' - 10;
      tp++;
   }

   if ((ULONG)((ULONG_PTR)tp - (ULONG_PTR)temp) >= Length)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }

   sp = String;
   while (tp > temp)
      *sp++ = *--tp;
   *sp = 0;

   return STATUS_SUCCESS;
}



/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlIntegerToUnicodeString(
   IN ULONG  Value,
   IN ULONG  Base, /* optional */
   IN OUT PUNICODE_STRING String)
{
   ANSI_STRING AnsiString;
   CHAR Buffer[33];
   NTSTATUS Status;

   Status = RtlIntegerToChar (Value,
                              Base,
                              sizeof(Buffer),
                              Buffer);
   if (NT_SUCCESS(Status))
   {
      AnsiString.Buffer = Buffer;
      AnsiString.Length = strlen (Buffer);
      AnsiString.MaximumLength = sizeof(Buffer);

      Status = RtlAnsiStringToUnicodeString (String,
                                             &AnsiString,
                                             FALSE);
   }

   return Status;
}


/*
* @implemented
*/
NTSTATUS
STDCALL
RtlInt64ToUnicodeString (
	IN ULONGLONG Value,
	IN ULONG Base OPTIONAL,
	IN OUT PUNICODE_STRING String
	)
{
   LARGE_INTEGER LargeInt;
   ANSI_STRING AnsiString;
   CHAR Buffer[33];
   NTSTATUS Status;

   LargeInt.QuadPart = Value;

   Status = RtlLargeIntegerToChar (&LargeInt,
                                   Base,
                                   sizeof(Buffer),
                                   Buffer);
   if (NT_SUCCESS(Status))
   {
      AnsiString.Buffer = Buffer;
      AnsiString.Length = strlen (Buffer);
      AnsiString.MaximumLength = sizeof(Buffer);

      Status = RtlAnsiStringToUnicodeString (String,
                                             &AnsiString,
                                             FALSE);
   }

   return Status;
}


/*
 * @implemented
 *
 * RETURNS
 *  TRUE if String2 contains String1 as a prefix.
 */
BOOLEAN
STDCALL
RtlPrefixString(
   PANSI_STRING String1,
   PANSI_STRING String2,
   BOOLEAN  CaseInsensitive)
{
   PCHAR pc1;
   PCHAR pc2;
   ULONG Length;

   if (String2->Length < String1->Length)
      return FALSE;

   Length = String1->Length;
   pc1 = String1->Buffer;
   pc2 = String2->Buffer;

   if (pc1 && pc2)
   {
      if (CaseInsensitive)
      {
         while (Length--)
         {
            if (RtlUpperChar (*pc1++) != RtlUpperChar (*pc2++))
               return FALSE;
         }
      }
      else
      {
         while (Length--)
         {
            if (*pc1++ != *pc2++)
               return FALSE;
         }
      }
      return TRUE;
   }
   return FALSE;
}


/*
 * @implemented
 *
 * RETURNS
 *  TRUE if String2 contains String1 as a prefix.
 */
BOOLEAN
STDCALL
RtlPrefixUnicodeString(
   PUNICODE_STRING String1,
   PUNICODE_STRING String2,
   BOOLEAN  CaseInsensitive)
{
   PWCHAR pc1;
   PWCHAR pc2;
   ULONG Length;

   if (String2->Length < String1->Length)
      return FALSE;

   Length = String1->Length / 2;
   pc1 = String1->Buffer;
   pc2  = String2->Buffer;

   if (pc1 && pc2)
   {
      if (CaseInsensitive)
      {
         while (Length--)
         {
            if (RtlUpcaseUnicodeChar (*pc1++)
                  != RtlUpcaseUnicodeChar (*pc2++))
               return FALSE;
         }
      }
      else
      {
         while (Length--)
         {
            if( *pc1++ != *pc2++ )
               return FALSE;
         }
      }
      return TRUE;
   }
   return FALSE;
}

/**************************************************************************
 *      RtlUnicodeStringToInteger (NTDLL.@)
 * @implemented
 * Converts an unicode string into its integer equivalent.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. value contains the converted number
 *  Failure: STATUS_INVALID_PARAMETER, if base is not 0, 2, 8, 10 or 16.
 *           STATUS_ACCESS_VIOLATION, if value is NULL.
 *
 * NOTES
 *  For base 0 it uses 10 as base and the string should be in the format
 *      "{whitespace} [+|-] [0[x|o|b]] {digits}".
 *  For other bases the string should be in the format
 *      "{whitespace} [+|-] {digits}".
 *  No check is made for value overflow, only the lower 32 bits are assigned.
 *  If str is NULL it crashes, as the native function does.
 *
 *  Note that regardless of success or failure status, we should leave the
 *  partial value in Value.  An error is never returned based on the chars
 *  in the string.
 *
 * DIFFERENCES
 *  This function does not read garbage on string length 0 as the native
 *  version does.
 */
NTSTATUS
STDCALL
RtlUnicodeStringToInteger(
    PUNICODE_STRING str, /* [I] Unicode string to be converted */
    ULONG base,                /* [I] Number base for conversion (allowed 0, 2, 8, 10 or 16) */
    PULONG value)              /* [O] Destination for the converted value */
{
    LPWSTR lpwstr = str->Buffer;
    USHORT CharsRemaining = str->Length / sizeof(WCHAR);
    WCHAR wchCurrent;
    int digit;
    ULONG RunningTotal = 0;
    char bMinus = 0;

    while (CharsRemaining >= 1 && *lpwstr <= ' ') {
	lpwstr++;
	CharsRemaining--;
    } /* while */

    if (CharsRemaining >= 1) {
	if (*lpwstr == '+') {
	    lpwstr++;
	    CharsRemaining--;
	} else if (*lpwstr == '-') {
	    bMinus = 1;
	    lpwstr++;
	    CharsRemaining--;
	} /* if */
    } /* if */

    if (base == 0) {
	base = 10;
	if (CharsRemaining >= 2 && lpwstr[0] == '0') {
	    if (lpwstr[1] == 'b') {
		lpwstr += 2;
		CharsRemaining -= 2;
		base = 2;
	    } else if (lpwstr[1] == 'o') {
		lpwstr += 2;
		CharsRemaining -= 2;
		base = 8;
	    } else if (lpwstr[1] == 'x') {
		lpwstr += 2;
		CharsRemaining -= 2;
		base = 16;
	    } /* if */
	} /* if */
    } else if (base != 2 && base != 8 && base != 10 && base != 16) {
	return STATUS_INVALID_PARAMETER;
    } /* if */

    if (value == NULL) {
	return STATUS_ACCESS_VIOLATION;
    } /* if */

    while (CharsRemaining >= 1) {
	wchCurrent = *lpwstr;
	if (wchCurrent >= '0' && wchCurrent <= '9') {
	    digit = wchCurrent - '0';
	} else if (wchCurrent >= 'A' && wchCurrent <= 'Z') {
	    digit = wchCurrent - 'A' + 10;
	} else if (wchCurrent >= 'a' && wchCurrent <= 'z') {
	    digit = wchCurrent - 'a' + 10;
	} else {
	    digit = -1;
	} /* if */
	if (digit < 0 || digit >= (int)base) {
	    *value = bMinus ? -RunningTotal : RunningTotal;
	    return STATUS_SUCCESS;
	} /* if */

	RunningTotal = RunningTotal * base + digit;
	lpwstr++;
	CharsRemaining--;
    } /* while */

    *value = bMinus ? -RunningTotal : RunningTotal;
    return STATUS_SUCCESS;
}


/*
 * @implemented
 *
 * RETURNS
 *  Bytes necessary for the conversion including nullterm.
 */
ULONG
STDCALL
RtlUnicodeStringToOemSize(
   IN PUNICODE_STRING UnicodeString)
{
   ULONG Size;

   RtlUnicodeToMultiByteSize (&Size,
                              UnicodeString->Buffer,
                              UnicodeString->Length);

   return Size+1; //NB: incl. nullterm
}

/*
 * @implemented
 *

 * NOTES
 *  This function always writes a terminating '\0'.
 *  It performs a partial copy if ansi is too small.
 */
NTSTATUS
STDCALL
RtlUnicodeStringToAnsiString(
   IN OUT PANSI_STRING AnsiDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString)
{
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG Length; /* including nullterm */

   if (NlsMbCodePageTag == TRUE)
   {
      Length = RtlUnicodeStringToAnsiSize(UniSource);
   }
   else
      Length = (UniSource->Length / sizeof(WCHAR)) + sizeof(CHAR);

   AnsiDest->Length = Length - sizeof(CHAR);

   if (AllocateDestinationString)
   {
      AnsiDest->Buffer = RtlpAllocateStringMemory(Length, TAG_ASTR);
      if (AnsiDest->Buffer == NULL)
         return STATUS_NO_MEMORY;

      AnsiDest->MaximumLength = Length;
   }
   else if (AnsiDest->MaximumLength == 0)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }
   else if (Length > AnsiDest->MaximumLength)
   {
      /* make room for nullterm */
      AnsiDest->Length = AnsiDest->MaximumLength - sizeof(CHAR);
   }

   Status = RtlUnicodeToMultiByteN (AnsiDest->Buffer,
                                    AnsiDest->Length,
                                    NULL,
                                    UniSource->Buffer,
                                    UniSource->Length);

   if (!NT_SUCCESS(Status) && AllocateDestinationString)
   {
      RtlpFreeStringMemory(AnsiDest->Buffer, TAG_ASTR);
      return Status;
   }

   AnsiDest->Buffer[AnsiDest->Length] = 0;
   return Status;
}


/*
 * @implemented
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 *  Does NOT perform a partial copy if unicode is too small!
 */
NTSTATUS
STDCALL
RtlOemStringToUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN POEM_STRING OemSource,
   IN BOOLEAN AllocateDestinationString)
{
   NTSTATUS Status;
   ULONG Length; /* including nullterm */

   if (NlsMbOemCodePageTag == TRUE)
      Length = RtlOemStringToUnicodeSize(OemSource);
   else
      Length = (OemSource->Length * sizeof(WCHAR)) + sizeof(WCHAR);

   if (Length > 0xffff)
      return STATUS_INVALID_PARAMETER_2;

   UniDest->Length = (WORD)(Length - sizeof(WCHAR));

   if (AllocateDestinationString)
   {
      UniDest->Buffer = RtlpAllocateStringMemory(Length, TAG_USTR);
      if (UniDest->Buffer == NULL)
         return STATUS_NO_MEMORY;

      UniDest->MaximumLength = Length;
   }
   else if (Length > UniDest->MaximumLength)
   {
      DPRINT("STATUS_BUFFER_TOO_SMALL\n");
      return STATUS_BUFFER_TOO_SMALL;
   }

   /* FIXME: Do we need this????? -Gunnar */
   RtlZeroMemory (UniDest->Buffer,
                  UniDest->Length);

   Status = RtlOemToUnicodeN (UniDest->Buffer,
                              UniDest->Length,
                              NULL,
                              OemSource->Buffer,
                              OemSource->Length);

   if (!NT_SUCCESS(Status) && AllocateDestinationString)
   {
      RtlpFreeStringMemory(UniDest->Buffer, TAG_USTR);
      return Status;
   }

   UniDest->Buffer[UniDest->Length / sizeof(WCHAR)] = 0;
   return STATUS_SUCCESS;
}


/*
 * @implemented
 *
 * NOTES
 *   This function always '\0' terminates the string returned.
 */
NTSTATUS
STDCALL
RtlUnicodeStringToOemString(
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString)
{
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG Length; //including nullterm

   if (NlsMbOemCodePageTag == TRUE)
      Length = RtlUnicodeStringToAnsiSize (UniSource);
   else
      Length = (UniSource->Length / sizeof(WCHAR)) + sizeof(CHAR);

   if (Length > 0x0000FFFF)
      return STATUS_INVALID_PARAMETER_2;

   OemDest->Length = (WORD)(Length - sizeof(CHAR));

   if (AllocateDestinationString)
   {
      OemDest->Buffer = RtlpAllocateStringMemory(Length, TAG_OSTR);
      if (OemDest->Buffer == NULL)
         return STATUS_NO_MEMORY;

      OemDest->MaximumLength = Length;
   }
   else if (OemDest->MaximumLength == 0)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }
   else if (Length > OemDest->MaximumLength)
   {
      //make room for nullterm
      OemDest->Length = OemDest->MaximumLength - sizeof(CHAR);
   }

   Status = RtlUnicodeToOemN (OemDest->Buffer,
                              OemDest->Length,
                              NULL,
                              UniSource->Buffer,
                              UniSource->Length);

   if (!NT_SUCCESS(Status) && AllocateDestinationString)
   {
      RtlpFreeStringMemory(OemDest->Buffer, TAG_OSTR);
      return Status;
   }

   OemDest->Buffer[OemDest->Length] = 0;
   return Status;
}

#define ITU_IMPLEMENTED_TESTS (IS_TEXT_UNICODE_ODD_LENGTH|IS_TEXT_UNICODE_SIGNATURE)


/*
 * @implemented
 *
 * RETURNS
 *  The length of the string if all tests were passed, 0 otherwise.
 */
ULONG STDCALL
RtlIsTextUnicode (PVOID Buffer,
                  ULONG Length,
                  ULONG *Flags)
{
   PWSTR s = Buffer;
   ULONG in_flags = (ULONG)-1;
   ULONG out_flags = 0;

   if (Length == 0)
      goto done;

   if (Flags != 0)
      in_flags = *Flags;

   /*
    * Apply various tests to the text string. According to the
    * docs, each test "passed" sets the corresponding flag in
    * the output flags. But some of the tests are mutually
    * exclusive, so I don't see how you could pass all tests ...
    */

   /* Check for an odd length ... pass if even. */
   if (!(Length & 1))
      out_flags |= IS_TEXT_UNICODE_ODD_LENGTH;

   /* Check for the BOM (byte order mark). */
   if (*s == 0xFEFF)
      out_flags |= IS_TEXT_UNICODE_SIGNATURE;

#if 0
   /* Check for the reverse BOM (byte order mark). */
   if (*s == 0xFFFE)
      out_flags |= IS_TEXT_UNICODE_REVERSE_SIGNATURE;
#endif

   /* FIXME: Add more tests */

   /*
    * Check whether the string passed all of the tests.
    */
   in_flags &= ITU_IMPLEMENTED_TESTS;
   if ((out_flags & in_flags) != in_flags)
      Length = 0;

done:
   if (Flags != 0)
      *Flags = out_flags;

   return Length;
}


/*
 * @implemented
 *
 * NOTES
 *  Same as RtlOemStringToUnicodeString but doesn't write terminating null
 *  A partial copy is NOT performed if the dest buffer is too small!
 */
NTSTATUS
STDCALL
RtlOemStringToCountedUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN POEM_STRING OemSource,
   IN BOOLEAN AllocateDestinationString)
{
   NTSTATUS Status;
   ULONG Length; /* excluding nullterm */

   if (NlsMbCodePageTag == TRUE)
      Length = RtlOemStringToUnicodeSize(OemSource) - sizeof(WCHAR);
   else
      Length = OemSource->Length * sizeof(WCHAR);

   if (Length > 65535)
      return STATUS_INVALID_PARAMETER_2;

   if (AllocateDestinationString == TRUE)
   {
      UniDest->Buffer = RtlpAllocateStringMemory (Length, TAG_USTR);
      if (UniDest->Buffer == NULL)
         return STATUS_NO_MEMORY;

      UniDest->MaximumLength = Length;
   }
   else if (Length > UniDest->MaximumLength)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }

   UniDest->Length = Length;

   Status = RtlOemToUnicodeN (UniDest->Buffer,
                              UniDest->Length,
                              NULL,
                              OemSource->Buffer,
                              OemSource->Length);

   if (!NT_SUCCESS(Status) && AllocateDestinationString)
   {
      RtlpFreeStringMemory(UniDest->Buffer, TAG_USTR);
      return Status;
   }

   return Status;
}

/*
 * @implemented
 *
 * RETURNS
 *  TRUE if the names are equal, FALSE if not
 *
 * NOTES
 *  The comparison is case insensitive.
 */
BOOLEAN
STDCALL
RtlEqualComputerName(
   IN PUNICODE_STRING ComputerName1,
   IN PUNICODE_STRING ComputerName2)
{
   OEM_STRING OemString1;
   OEM_STRING OemString2;
   BOOLEAN Result = FALSE;

   if (NT_SUCCESS(RtlUpcaseUnicodeStringToOemString( &OemString1, ComputerName1, TRUE )))
   {
      if (NT_SUCCESS(RtlUpcaseUnicodeStringToOemString( &OemString2, ComputerName2, TRUE )))
      {
         Result = RtlEqualString( &OemString1, &OemString2, TRUE );
         RtlFreeOemString( &OemString2 );
      }
      RtlFreeOemString( &OemString1 );
   }

   return Result;
}

/*
 * @implemented
 *
 * RETURNS
 *  TRUE if the names are equal, FALSE if not
 *
 * NOTES
 *  The comparison is case insensitive.
 */
BOOLEAN
STDCALL
RtlEqualDomainName (
   IN PUNICODE_STRING DomainName1,
   IN PUNICODE_STRING DomainName2
)
{
   return RtlEqualComputerName(DomainName1, DomainName2);
}


/*
 * @implemented
 */
/*
BOOLEAN
STDCALL
RtlEqualDomainName (
   IN PUNICODE_STRING DomainName1,
   IN PUNICODE_STRING DomainName2
)
{
   OEM_STRING OemString1;
   OEM_STRING OemString2;
   BOOLEAN Result;

   RtlUpcaseUnicodeStringToOemString (&OemString1,
                                      DomainName1,
                                      TRUE);
   RtlUpcaseUnicodeStringToOemString (&OemString2,
                                      DomainName2,
                                      TRUE);

   Result = RtlEqualString (&OemString1,
                            &OemString2,
                            FALSE);

   RtlFreeOemString (&OemString1);
   RtlFreeOemString (&OemString2);

   return Result;

}
*/

/*
 * @implemented
 *
 * RIPPED FROM WINE's ntdll\rtlstr.c rev 1.45
 *
 * Convert a string representation of a GUID into a GUID.
 *
 * PARAMS
 *  str  [I] String representation in the format "{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}"
 *  guid [O] Destination for the converted GUID
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. guid contains the converted value.
 *  Failure: STATUS_INVALID_PARAMETER, if str is not in the expected format.
 *
 * SEE ALSO
 *  See RtlStringFromGUID.
 */
NTSTATUS
STDCALL
RtlGUIDFromString(
   IN UNICODE_STRING *str,
   OUT GUID* guid
)
{
   int i = 0;
   const WCHAR *lpszCLSID = str->Buffer;
   BYTE* lpOut = (BYTE*)guid;

   //TRACE("(%s,%p)\n", debugstr_us(str), guid);

   /* Convert string: {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}
    * to memory:       DWORD... WORD WORD BYTES............
    */
   while (i < 37)
   {
      switch (i)
      {
         case 0:
            if (*lpszCLSID != '{')
               return STATUS_INVALID_PARAMETER;
            break;

         case 9:
         case 14:
         case 19:
         case 24:
            if (*lpszCLSID != '-')
               return STATUS_INVALID_PARAMETER;
            break;

         case 37:
            if (*lpszCLSID != '}')
               return STATUS_INVALID_PARAMETER;
            break;

         default:
            {
               WCHAR ch = *lpszCLSID, ch2 = lpszCLSID[1];
               unsigned char byte;

               /* Read two hex digits as a byte value */
               if      (ch >= '0' && ch <= '9')
                  ch = ch - '0';
               else if (ch >= 'a' && ch <= 'f')
                  ch = ch - 'a' + 10;
               else if (ch >= 'A' && ch <= 'F')
                  ch = ch - 'A' + 10;
               else
                  return STATUS_INVALID_PARAMETER;

               if      (ch2 >= '0' && ch2 <= '9')
                  ch2 = ch2 - '0';
               else if (ch2 >= 'a' && ch2 <= 'f')
                  ch2 = ch2 - 'a' + 10;
               else if (ch2 >= 'A' && ch2 <= 'F')
                  ch2 = ch2 - 'A' + 10;
               else
                  return STATUS_INVALID_PARAMETER;

               byte = ch << 4 | ch2;

               switch (i)
               {
#ifndef WORDS_BIGENDIAN
                     /* For Big Endian machines, we store the data such that the
                      * dword/word members can be read as DWORDS and WORDS correctly. */
                     /* Dword */
                  case 1:
                     lpOut[3] = byte;
                     break;
                  case 3:
                     lpOut[2] = byte;
                     break;
                  case 5:
                     lpOut[1] = byte;
                     break;
                  case 7:
                     lpOut[0] = byte;
                     lpOut += 4;
                     break;
                     /* Word */
                  case 10:
                  case 15:
                     lpOut[1] = byte;
                     break;
                  case 12:
                  case 17:
                     lpOut[0] = byte;
                     lpOut += 2;
                     break;
#endif
                     /* Byte */
                  default:
                     lpOut[0] = byte;
                     lpOut++;
                     break;
               }
               lpszCLSID++; /* Skip 2nd character of byte */
               i++;
            }
      }
      lpszCLSID++;
      i++;
   }

   return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
STDCALL
RtlEraseUnicodeString(
   IN PUNICODE_STRING String)
{
   if (String->Buffer != NULL &&
       String->MaximumLength != 0)
   {
      RtlZeroMemory (String->Buffer,
                     String->MaximumLength);

      String->Length = 0;
   }
}

/*
* @implemented
*/
NTSTATUS
STDCALL
RtlHashUnicodeString(
  IN CONST UNICODE_STRING *String,
  IN BOOLEAN CaseInSensitive,
  IN ULONG HashAlgorithm,
  OUT PULONG HashValue)
{
    if (String != NULL && HashValue != NULL)
    {
        switch (HashAlgorithm)
        {
            case HASH_STRING_ALGORITHM_DEFAULT:
            case HASH_STRING_ALGORITHM_X65599:
            {
                WCHAR *c, *end;

                *HashValue = 0;
                end = String->Buffer + (String->Length / sizeof(WCHAR));

                if (CaseInSensitive)
                {
                    for (c = String->Buffer;
                         c != end;
                         c++)
                    {
                        /* only uppercase characters if they are 'a' ... 'z'! */
                        *HashValue = ((65599 * (*HashValue)) +
                                      (ULONG)(((*c) >= L'a' && (*c) <= L'z') ?
                                              (*c) - L'a' + L'A' : (*c)));
                    }
                }
                else
                {
                    for (c = String->Buffer;
                         c != end;
                         c++)
                    {
                        *HashValue = ((65599 * (*HashValue)) + (ULONG)(*c));
                    }
                }
                return STATUS_SUCCESS;
            }
        }
    }

    return STATUS_INVALID_PARAMETER;
}

/*
 * @implemented
 *
 * NOTES
 *  Same as RtlUnicodeStringToOemString but doesn't write terminating null
 *  Does a partial copy if the dest buffer is too small
 */
NTSTATUS
STDCALL
RtlUnicodeStringToCountedOemString(
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString)
{
   NTSTATUS Status;
   ULONG Length; //excluding nullterm

   if (NlsMbOemCodePageTag == TRUE)
      Length = RtlUnicodeStringToAnsiSize(UniSource) - sizeof(CHAR);
   else
      Length = (UniSource->Length / sizeof(WCHAR));

   if (Length > 0x0000FFFF)
      return STATUS_INVALID_PARAMETER_2;

   OemDest->Length = (WORD)(Length);

   if (AllocateDestinationString)
   {
      OemDest->Buffer = RtlpAllocateStringMemory(Length, TAG_OSTR);
      if (OemDest->Buffer == NULL)
         return STATUS_NO_MEMORY;

      OemDest->MaximumLength = Length;
   }
   else if (OemDest->MaximumLength == 0)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }
   else if (Length > OemDest->MaximumLength)
   {
      OemDest->Length = OemDest->MaximumLength;
   }

   Status = RtlUnicodeToOemN (OemDest->Buffer,
                              OemDest->Length,
                              NULL,
                              UniSource->Buffer,
                              UniSource->Length);

   if (!NT_SUCCESS(Status) && AllocateDestinationString)
   {
      RtlpFreeStringMemory(OemDest->Buffer, TAG_OSTR);
   }

   return Status;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlLargeIntegerToChar(
   IN PLARGE_INTEGER Value,
   IN ULONG  Base,
   IN ULONG  Length,
   IN OUT PCHAR  String)
{
   ULONG Radix;
   CHAR  temp[65];
   ULONGLONG v = Value->QuadPart;
   ULONG i;
   PCHAR tp;
   PCHAR sp;

   Radix = Base;
   if (Radix == 0)
      Radix = 10;

   if ((Radix != 2) && (Radix != 8) &&
         (Radix != 10) && (Radix != 16))
      return STATUS_INVALID_PARAMETER;

   tp = temp;
   while (v || tp == temp)
   {
      i = v % Radix;
      v = v / Radix;
      if (i < 10)
         *tp = i + '0';
      else
         *tp = i + 'a' - 10;
      tp++;
   }

   if ((ULONG)((ULONG_PTR)tp - (ULONG_PTR)temp) >= Length)
      return STATUS_BUFFER_TOO_SMALL;

   sp = String;
   while (tp > temp)
      *sp++ = *--tp;
   *sp = 0;

   return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * NOTES
 *  dest is never '\0' terminated because it may be equal to src, and src
 *  might not be '\0' terminated. dest->Length is only set upon success.
 */
NTSTATUS
STDCALL
RtlUpcaseUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString)
{
   ULONG i;
   PWCHAR Src, Dest;

   if (AllocateDestinationString == TRUE)
   {
      UniDest->MaximumLength = UniSource->Length;
      UniDest->Buffer = RtlpAllocateStringMemory(UniDest->MaximumLength, TAG_USTR);
      if (UniDest->Buffer == NULL)
         return STATUS_NO_MEMORY;
   }
   else if (UniSource->Length > UniDest->MaximumLength)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }

   UniDest->Length = UniSource->Length;

   Src = UniSource->Buffer;
   Dest = UniDest->Buffer;
   for (i = 0; i < UniSource->Length / sizeof(WCHAR); i++)
   {
      *Dest = RtlUpcaseUnicodeChar (*Src);
      Dest++;
      Src++;
   }

   return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 *  It performs a partial copy if ansi is too small.
 */
NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToAnsiString(
   IN OUT PANSI_STRING AnsiDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString)
{
   NTSTATUS Status;
   ULONG Length; /* including nullterm */

   if (NlsMbCodePageTag == TRUE)
      Length = RtlUnicodeStringToAnsiSize(UniSource);
   else
      Length = (UniSource->Length / sizeof(WCHAR)) + sizeof(CHAR);

   if (Length > 0x0000FFFF)
      return STATUS_INVALID_PARAMETER_2;

   AnsiDest->Length = (WORD)(Length - sizeof(CHAR));

   if (AllocateDestinationString)
   {
      AnsiDest->Buffer = RtlpAllocateStringMemory(Length, TAG_ASTR);
      if (AnsiDest->Buffer == NULL)
         return STATUS_NO_MEMORY;

      AnsiDest->MaximumLength = Length;
   }
   else if (AnsiDest->MaximumLength == 0)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }
   else if (Length > AnsiDest->MaximumLength)
   {
      /* make room for nullterm */
      AnsiDest->Length = AnsiDest->MaximumLength - sizeof(CHAR);
   }

   /* FIXME: do we need this??????? -Gunnar */
   RtlZeroMemory (AnsiDest->Buffer,
                  AnsiDest->Length);

   Status = RtlUpcaseUnicodeToMultiByteN (AnsiDest->Buffer,
                                          AnsiDest->Length,
                                          NULL,
                                          UniSource->Buffer,
                                          UniSource->Length);

   if (!NT_SUCCESS(Status) && AllocateDestinationString)
   {
      RtlpFreeStringMemory(AnsiDest->Buffer, TAG_ASTR);
      return Status;
   }

   AnsiDest->Buffer[AnsiDest->Length] = 0;
   return Status;
}

/*
 * @implemented
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 *  It performs a partial copy if ansi is too small.
 */
NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToCountedOemString(
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString)
{
   NTSTATUS Status;
   ULONG Length; /* excluding nullterm */

   if (NlsMbCodePageTag == TRUE)
      Length = RtlUnicodeStringToAnsiSize(UniSource) - sizeof(CHAR);
   else
      Length = UniSource->Length / sizeof(WCHAR);

   if (Length > 0x0000FFFF)
      return(STATUS_INVALID_PARAMETER_2);

   OemDest->Length = (WORD)(Length);

   if (AllocateDestinationString)
   {
      OemDest->Buffer = RtlpAllocateStringMemory(Length, TAG_OSTR);
      if (OemDest->Buffer == NULL)
         return(STATUS_NO_MEMORY);

      /* FIXME: Do we need this????? */
      RtlZeroMemory (OemDest->Buffer, Length);

      OemDest->MaximumLength = (WORD)Length;
   }
   else if (OemDest->MaximumLength == 0)
   {
      return(STATUS_BUFFER_TOO_SMALL);
   }
   else if (Length > OemDest->MaximumLength)
   {
      OemDest->Length = OemDest->MaximumLength;
   }

   Status = RtlUpcaseUnicodeToOemN(OemDest->Buffer,
                                   OemDest->Length,
                                   NULL,
                                   UniSource->Buffer,
                                   UniSource->Length);

   if (!NT_SUCCESS(Status) && AllocateDestinationString)
   {
      RtlpFreeStringMemory(OemDest->Buffer, TAG_OSTR);
      return Status;
   }

   return Status;
}

/*
 * @implemented
 * NOTES
 *  Oem string is allways nullterminated
 *  It performs a partial copy if oem is too small.
 */
NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToOemString (
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString
)
{
   NTSTATUS Status;
   ULONG Length; /* including nullterm */

   if (NlsMbOemCodePageTag == TRUE)
      Length = RtlUnicodeStringToAnsiSize(UniSource);
   else
      Length = (UniSource->Length / sizeof(WCHAR)) + sizeof(CHAR);

   if (Length > 0x0000FFFF)
      return STATUS_INVALID_PARAMETER_2;

   OemDest->Length = (WORD)(Length - sizeof(CHAR));

   if (AllocateDestinationString)
   {
      OemDest->Buffer = RtlpAllocateStringMemory(Length, TAG_OSTR);
      if (OemDest->Buffer == NULL)
         return STATUS_NO_MEMORY;

      /* FIXME: Do we need this???? */
      RtlZeroMemory (OemDest->Buffer, Length);

      OemDest->MaximumLength = (WORD)Length;
   }
   else if (OemDest->MaximumLength == 0)
   {
      return STATUS_BUFFER_OVERFLOW;
   }
   else if (Length > OemDest->MaximumLength)
   {
      /* make room for nullterm */
      OemDest->Length = OemDest->MaximumLength - sizeof(CHAR);
   }

   Status = RtlUpcaseUnicodeToOemN (OemDest->Buffer,
                                    OemDest->Length,
                                    NULL,
                                    UniSource->Buffer,
                                    UniSource->Length);

   if (!NT_SUCCESS(Status) && AllocateDestinationString)
   {
      RtlpFreeStringMemory(OemDest->Buffer, TAG_OSTR);
      return Status;
   }

   OemDest->Buffer[OemDest->Length] = 0;
   return Status;
}

/*
 * @implemented
 *
 * RETURNS
 *  Bytes calculated including nullterm
 */
ULONG
STDCALL
RtlOemStringToUnicodeSize(IN POEM_STRING OemString)
{
   ULONG Size;

   //this function returns size including nullterm
   RtlMultiByteToUnicodeSize(&Size,
                             OemString->Buffer,
                             OemString->Length);

   return(Size);
}



/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlStringFromGUID (IN REFGUID Guid,
                   OUT PUNICODE_STRING GuidString)
{
   STATIC CONST PWCHAR Hex = L"0123456789ABCDEF";
   WCHAR Buffer[40];
   PWCHAR BufferPtr;
   ULONG i;

   if (Guid == NULL)
   {
      return STATUS_INVALID_PARAMETER;
   }

   swprintf (Buffer,
             L"{%08lX-%04X-%04X-%02X%02X-",
             Guid->Data1,
             Guid->Data2,
             Guid->Data3,
             Guid->Data4[0],
             Guid->Data4[1]);
   BufferPtr = Buffer + 25;

   /* 6 hex bytes */
   for (i = 2; i < 8; i++)
   {
      *BufferPtr++ = Hex[Guid->Data4[i] >> 4];
      *BufferPtr++ = Hex[Guid->Data4[i] & 0xf];
   }

   *BufferPtr++ = L'}';
   *BufferPtr++ = L'\0';

   return RtlCreateUnicodeString (GuidString, Buffer);
}


/*
 * @implemented
 *
 * RETURNS
 *  Bytes calculated including nullterm
 */
ULONG
STDCALL
RtlUnicodeStringToAnsiSize(
   IN PUNICODE_STRING UnicodeString)
{
   ULONG Size;

   //this function return size without nullterm!
   RtlUnicodeToMultiByteSize (&Size,
                              UnicodeString->Buffer,
                              UnicodeString->Length);

   return Size + sizeof(CHAR); //NB: incl. nullterm
}




/*
 * @implemented
 */
LONG
STDCALL
RtlCompareUnicodeString(
   IN PUNICODE_STRING String1,
   IN PUNICODE_STRING String2,
   IN BOOLEAN  CaseInsensitive)
{
   ULONG len1, len2;
   PWCHAR s1, s2;
   WCHAR  c1, c2;

   if (String1 && String2)
   {
      len1 = String1->Length / sizeof(WCHAR);
      len2 = String2->Length / sizeof(WCHAR);
      s1 = String1->Buffer;
      s2 = String2->Buffer;

      if (s1 && s2)
      {
         if (CaseInsensitive)
         {
            while (1)
            {
               c1 = len1-- ? RtlUpcaseUnicodeChar (*s1++) : 0;
               c2 = len2-- ? RtlUpcaseUnicodeChar (*s2++) : 0;
               if (!c1 || !c2 || c1 != c2)
                  return c1 - c2;
            }
         }
         else
         {
            while (1)
            {
               c1 = len1-- ? *s1++ : 0;
               c2 = len2-- ? *s2++ : 0;
               if (!c1 || !c2 || c1 != c2)
                  return c1 - c2;
            }
         }
      }
   }

   return 0;
}


/*
 * @implemented
 */
VOID
STDCALL
RtlCopyString(
   IN OUT PSTRING DestinationString,
   IN PSTRING SourceString)
{
   ULONG copylen;

   if(SourceString == NULL)
   {
      DestinationString->Length = 0;
      return;
   }

   copylen = min (DestinationString->MaximumLength,
                  SourceString->Length);

   memcpy(DestinationString->Buffer, SourceString->Buffer, copylen);
   if (DestinationString->MaximumLength >= copylen + sizeof(CHAR))
   {
      DestinationString->Buffer[copylen] = 0;
   }
   DestinationString->Length = copylen;
}



/*
 * @implemented
 */
VOID
STDCALL
RtlCopyUnicodeString(
   IN OUT PUNICODE_STRING DestinationString,
   IN PUNICODE_STRING SourceString)
{
   ULONG copylen;

   if (SourceString == NULL)
   {
      DestinationString->Length = 0;
      return;
   }

   copylen = min (DestinationString->MaximumLength,
                  SourceString->Length);
   memcpy(DestinationString->Buffer, SourceString->Buffer, copylen);
   if (DestinationString->MaximumLength >= copylen + sizeof(WCHAR))
   {
     DestinationString->Buffer[copylen / sizeof(WCHAR)] = 0;
   }
   DestinationString->Length = copylen;
}

/*
 * @implemented
 *
 * NOTES
 * Creates a nullterminated UNICODE_STRING
 */
BOOLEAN
STDCALL
RtlCreateUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCWSTR  Source)
{
   ULONG Length;

   Length = (wcslen (Source) + 1) * sizeof(WCHAR);
   UniDest->Buffer = RtlpAllocateStringMemory(Length, TAG_USTR);
   if (UniDest->Buffer == NULL)
      return FALSE;

   RtlCopyMemory (UniDest->Buffer,
                  Source,
                  Length);

   UniDest->MaximumLength = Length;
   UniDest->Length = Length - sizeof (WCHAR);

   return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
RtlCreateUnicodeStringFromAsciiz(
   OUT PUNICODE_STRING Destination,
   IN PCSZ  Source)
{
   ANSI_STRING AnsiString;
   NTSTATUS Status;

   RtlInitAnsiString (&AnsiString,
                      Source);

   Status = RtlAnsiStringToUnicodeString (Destination,
                                          &AnsiString,
                                          TRUE);

   return NT_SUCCESS(Status);
}

/*
 * @implemented
 *
 * NOTES
 *  Dest is never '\0' terminated because it may be equal to src, and src
 *  might not be '\0' terminated.
 *  Dest->Length is only set upon success.
 */
NTSTATUS STDCALL
RtlDowncaseUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString)
{
   ULONG i;
   PWCHAR Src, Dest;

   if (AllocateDestinationString)
   {
      UniDest->Buffer = RtlpAllocateStringMemory(UniSource->Length, TAG_USTR);
      if (UniDest->Buffer == NULL)
         return STATUS_NO_MEMORY;

      UniDest->MaximumLength = UniSource->Length;
   }
   else if (UniSource->Length > UniDest->MaximumLength)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }

   UniDest->Length = UniSource->Length;

   Src = UniSource->Buffer;
   Dest = UniDest->Buffer;
   for (i=0; i < UniSource->Length / sizeof(WCHAR); i++)
   {
      if (*Src < L'A')
      {
         *Dest = *Src;
      }
      else if (*Src <= L'Z')
      {
         *Dest = (*Src + (L'a' - L'A'));
      }
      else
      {
         *Dest = RtlDowncaseUnicodeChar(*Src);
      }

      Dest++;
      Src++;
   }

   return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * NOTES
 *  if src is NULL dest is unchanged.
 *  dest is '\0' terminated when the MaximumLength allowes it.
 *  When dest fits exactly in MaximumLength characters the '\0' is ommitted.
 */
NTSTATUS STDCALL
RtlAppendUnicodeToString(IN OUT PUNICODE_STRING Destination,
                         IN PCWSTR Source)
{
   ULONG slen;

   slen = wcslen(Source) * sizeof(WCHAR);

   if (Destination->Length + slen > Destination->MaximumLength)
      return(STATUS_BUFFER_TOO_SMALL);

   memcpy((char*)Destination->Buffer + Destination->Length, Source, slen);
   Destination->Length += slen;
   /* append terminating '\0' if enough space */
   if( Destination->MaximumLength > Destination->Length )
      Destination->Buffer[Destination->Length / sizeof(WCHAR)] = 0;

   return(STATUS_SUCCESS);
}

/*
 * @implemented
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 *  If the dest buffer is too small a partial copy is NOT performed!
 */
NTSTATUS
STDCALL
RtlAnsiStringToUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PANSI_STRING AnsiSource,
   IN BOOLEAN AllocateDestinationString)
{
   NTSTATUS Status;
   ULONG Length; //including nullterm

   if (NlsMbCodePageTag == TRUE)
      Length = RtlAnsiStringToUnicodeSize(AnsiSource);
   else
      Length = (AnsiSource->Length * sizeof(WCHAR)) + sizeof(WCHAR);

   if (Length > 0xffff)
      return STATUS_INVALID_PARAMETER_2;

   if (AllocateDestinationString == TRUE)
   {
      UniDest->Buffer = RtlpAllocateStringMemory(Length, TAG_USTR);
      if (UniDest->Buffer == NULL)
         return STATUS_NO_MEMORY;

      UniDest->MaximumLength = Length;
   }
   else if (Length > UniDest->MaximumLength)
   {
      DPRINT("STATUS_BUFFER_TOO_SMALL\n");
      return STATUS_BUFFER_TOO_SMALL;
   }

   UniDest->Length = Length - sizeof(WCHAR);

   //FIXME: We don't need this??? -Gunnar
   RtlZeroMemory (UniDest->Buffer,
                  UniDest->Length);

   Status = RtlMultiByteToUnicodeN (UniDest->Buffer,
                                    UniDest->Length,
                                    NULL,
                                    AnsiSource->Buffer,
                                    AnsiSource->Length);

   if (!NT_SUCCESS(Status) && AllocateDestinationString)
   {
      RtlpFreeStringMemory(UniDest->Buffer, TAG_USTR);
      return Status;
   }

   UniDest->Buffer[UniDest->Length / sizeof(WCHAR)] = 0;
   return Status;
}

/*
 * @implemented
 *
 * NOTES
 *  if src is NULL dest is unchanged.
 *  dest is never '\0' terminated.
 */
NTSTATUS
STDCALL
RtlAppendAsciizToString(
   IN OUT   PSTRING  Destination,
   IN PCSZ  Source)
{
   ULONG Length;
   PCHAR Ptr;

   if (Source == NULL)
      return STATUS_SUCCESS;

   Length = strlen (Source);
   if (Destination->Length + Length >= Destination->MaximumLength)
      return STATUS_BUFFER_TOO_SMALL;

   Ptr = Destination->Buffer + Destination->Length;
   memmove (Ptr,
            Source,
            Length);
   Ptr += Length;
   *Ptr = 0;

   Destination->Length += Length;

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
VOID STDCALL
RtlUpperString(PSTRING DestinationString,
               PSTRING SourceString)
{
   ULONG Length;
   ULONG i;
   PCHAR Src;
   PCHAR Dest;

   Length = min(SourceString->Length,
                DestinationString->MaximumLength - 1);

   Src = SourceString->Buffer;
   Dest = DestinationString->Buffer;
   for (i = 0; i < Length; i++)
   {
      *Dest = RtlUpperChar(*Src);
      Src++;
      Dest++;
   }
   *Dest = 0;

   DestinationString->Length = SourceString->Length;
}


/*
 * @implemented
 */
ULONG STDCALL
RtlxAnsiStringToUnicodeSize(IN PANSI_STRING AnsiString)
{
   return RtlAnsiStringToUnicodeSize(AnsiString);
}


/*
 * @implemented
 */
ULONG STDCALL
RtlxOemStringToUnicodeSize(IN POEM_STRING OemString)
{
   return RtlOemStringToUnicodeSize(OemString);
}



/*
 * @implemented
 */
ULONG STDCALL
RtlxUnicodeStringToAnsiSize(IN PUNICODE_STRING UnicodeString)
{
   return RtlUnicodeStringToAnsiSize(UnicodeString);
}


/*
 * @implemented
 */
ULONG STDCALL
RtlxUnicodeStringToOemSize(IN PUNICODE_STRING UnicodeString)
{
   return RtlUnicodeStringToOemSize(UnicodeString);
}

/*
 * @implemented
 *
 * NOTES
 *  See RtlpDuplicateUnicodeString
 */
NTSTATUS STDCALL
RtlDuplicateUnicodeString(
   INT AddNull,
   IN PUNICODE_STRING SourceString,
   PUNICODE_STRING DestinationString)
{
   if (SourceString == NULL || DestinationString == NULL)
      return STATUS_INVALID_PARAMETER;


   if (SourceString->Length == 0 && AddNull != 3)
   {
      DestinationString->Length = 0;
      DestinationString->MaximumLength = 0;
      DestinationString->Buffer = NULL;
   }
   else
   {
      UINT DestMaxLength = SourceString->Length;

      if (AddNull)
         DestMaxLength += sizeof(UNICODE_NULL);

      DestinationString->Buffer = RtlpAllocateStringMemory(DestMaxLength, TAG_USTR);
      if (DestinationString->Buffer == NULL)
         return STATUS_NO_MEMORY;

      RtlCopyMemory(DestinationString->Buffer, SourceString->Buffer, SourceString->Length);
      DestinationString->Length = SourceString->Length;
      DestinationString->MaximumLength = DestMaxLength;

      if (AddNull)
         DestinationString->Buffer[DestinationString->Length / sizeof(WCHAR)] = 0;
   }

   return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
RtlValidateUnicodeString(IN ULONG Flags,
                         IN PUNICODE_STRING UnicodeString)
{
  /* currently no flags are supported! */
  ASSERT(Flags == 0);

  if ((Flags == 0) &&
      ((UnicodeString == NULL) ||
       ((UnicodeString->Length != 0) &&
        (UnicodeString->Buffer != NULL) &&
        ((UnicodeString->Length % sizeof(WCHAR)) == 0) &&
        ((UnicodeString->MaximumLength % sizeof(WCHAR)) == 0) &&
        (UnicodeString->MaximumLength >= UnicodeString->Length))))
  {
    /* a NULL pointer as a unicode string is considered to be a valid unicode
       string! */
    return STATUS_SUCCESS;
  }
  else
  {
    return STATUS_INVALID_PARAMETER;
  }
}

/* EOF */
