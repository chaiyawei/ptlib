/*
 * $Id: object.cxx,v 1.15 1996/01/02 12:52:02 robertj Exp $
 *
 * Portable Windows Library
 *
 * PContainer Class Implementation
 *
 * Copyright 1993 Equivalence
 *
 * $Log: object.cxx,v $
 * Revision 1.15  1996/01/02 12:52:02  robertj
 * Mac OS compatibility changes.
 *
 * Revision 1.14  1995/11/21 11:51:54  robertj
 * Improved streams compatibility.
 *
 * Revision 1.12  1995/04/25 11:30:34  robertj
 * Fixed Borland compiler warnings.
 * Fixed function hiding ancestors virtual.
 *
 * Revision 1.11  1995/03/12 04:59:53  robertj
 * Re-organisation of DOS/WIN16 and WIN32 platforms to maximise common code.
 * Used built-in equate for WIN32 API (_WIN32).
 *
 * Revision 1.10  1995/02/19  04:19:21  robertj
 * Added dynamically linked command processing.
 *
 * Revision 1.9  1995/01/15  04:52:02  robertj
 * Mac compatibility.
 * Added memory stats function.
 *
// Revision 1.8  1995/01/09  12:38:07  robertj
// Changed variable names around during documentation run.
// Fixed smart pointer comparison.
// Fixed serialisation stuff.
//
// Revision 1.7  1995/01/07  04:39:45  robertj
// Redesigned font enumeration code and changed font styles.
//
// Revision 1.6  1995/01/04  10:57:08  robertj
// Changed for HPUX and GNU2.6.x
//
// Revision 1.5  1995/01/03  09:39:10  robertj
// Put standard malloc style memory allocation etc into memory check system.
//
// Revision 1.4  1994/12/21  11:43:29  robertj
// Added extra memory stats.
//
// Revision 1.3  1994/12/13  11:54:54  robertj
// Added some memory usage statistics.
//
// Revision 1.2  1994/12/12  10:08:32  robertj
// Renamed PWrapper to PSmartPointer..
//
// Revision 1.1  1994/10/30  12:02:15  robertj
// Initial revision
//
 */

#include <contain.h>
#include <ctype.h>


void PAssertFunc(const char * file, int line, PStandardAssertMessage msg)
{
  static const char * const textmsg[PMaxStandardAssertMessage] = {
    NULL,
    "Out of memory",
    "Null pointer reference",
    "Invalid array index",
    "Invalid array element",
    "Stack empty",
    "Unimplemented function",
    "Invalid parameter",
    "Operating System error",
    "File not open",
    "Unsupported feature",
    "Invalid or closed operating system window"
  };

  const char * theMsg;
  char msgbuf[20];
  if (msg < PMaxStandardAssertMessage)
    theMsg = textmsg[msg];
  else {
    sprintf(msgbuf, "Error code: %i", msg);
    theMsg = msgbuf;
  }
  PAssertFunc(file, line, theMsg);
}


#ifdef PMEMORY_CHECK

static const size_t PointerTableSize = 12345;
static void **      PointerHashTable;
static long CurrentMemory;
static long PeakMemory;
static long CurrentObjects;
static long PeakObjects;
static long TotalObjects;

void PObject::MemoryCheckStatistics(long * currentMemory, long * peakMemory,
                long * currentObjects, long * peakObjects, long * totalObjects)
{
  if (currentMemory != NULL)
    *currentMemory = CurrentMemory;
  if (peakMemory != NULL)
    *peakMemory = PeakMemory;
  if (currentObjects != NULL)
    *currentObjects = CurrentObjects;
  if (peakObjects != NULL)
    *peakObjects = PeakObjects;
  if (totalObjects != NULL)
    *totalObjects = TotalObjects;
}


static const char GuardBytes[] = { '\x55', '\xaa', '\xff', '\xaa', '\x55' };

struct PointerArenaStruct {
  size_t       size;
  const char * fileName;
  int          line;
  const char * className;
  char         guard[sizeof(GuardBytes)];
};


#ifndef __BORLANDC__
static ostream & operator<<(ostream & out, void * ptr)
{
  return out << resetiosflags(ios::basefield) << setiosflags(ios::hex)
             << setw(8) << setfill('0') << (long)ptr
             << resetiosflags(ios::basefield) << setiosflags(ios::dec);
}
#endif


static void DumpMemoryLeaks()
{
  PError.setf(ios::uppercase);
  void ** entry = PointerHashTable;
  for (size_t i = 0; i < PointerTableSize; i++, entry++) {
    if (*entry != NULL) {
      PointerArenaStruct * arena = ((PointerArenaStruct *)*entry)-1;
      PError << "Pointer @" << (void *)(*entry)
             << " [" << arena->size << ']';
      if (arena->className != NULL)
        PError << " \"" << arena->className << '"';
      PError << " not deallocated.";
      if (arena->fileName != NULL)
        PError << " PNEW: " << arena->fileName << '(' << arena->line << ')';
      PError << endl;
    }
  }

  PError << "Peak memory usage: " << PeakMemory << " bytes";
  if (PeakMemory > 2048)
    PError << ", " << (PeakMemory+1023)/1024 << "kb";
  if (PeakMemory > 2097152)
    PError << ", " << (PeakMemory+1048575)/1048576 << "Mb";
  PError << ".\n"
            "Peak objects created: " << PeakObjects << "\n"
            "Total objects created: " << TotalObjects << endl;

#if defined(_WIN32)
  extern void PWaitOnExitConsoleWindow();
  PWaitOnExitConsoleWindow();
#endif
}


static void ** FindPointerInHashTable(void * object, void * search)
{
  int hash = (int)(((((long)object)&0x7fffffff)>>3)%PointerTableSize);
  void ** forward = &PointerHashTable[hash];
  void ** backward = forward;
  static void ** endHashTable = &PointerHashTable[PointerTableSize-1];

  do {
    if (*forward == search)
      return forward;

    if (forward == endHashTable)
      forward = PointerHashTable;
    else
      forward++;

    if (*backward == search)
      return backward;

    if (backward == PointerHashTable)
      backward = endHashTable;
    else
      backward--;
  } while (forward != backward);

  return NULL;
}


void * PObject::MemoryCheckAllocate(size_t nSize,
                           const char * file, int line, const char * className)
{
  if (PointerHashTable == NULL) {
    PointerHashTable = (void **)calloc(PointerTableSize, sizeof(void *));
    atexit(DumpMemoryLeaks);
  }

  PointerArenaStruct * arena = (PointerArenaStruct *)malloc(
                      sizeof(PointerArenaStruct) + nSize + sizeof(GuardBytes));
  if (arena == NULL) {
    PAssertAlways(POutOfMemory);
    return NULL;
  }

  TotalObjects++;
  if (++CurrentObjects > PeakObjects)
    PeakObjects = CurrentObjects;

  char * data = (char *)&arena[1];

  arena->size      = nSize;
  arena->fileName  = file;
  arena->line      = line;
  arena->className = className;
  memcpy(arena->guard, GuardBytes, sizeof(GuardBytes));
  memcpy(&data[nSize], GuardBytes, sizeof(GuardBytes));

  if (PointerHashTable != NULL) {
    void ** entry = FindPointerInHashTable(data, NULL);
    if (entry == NULL)
      PError << "Pointer @" << (void *)data
             << " not added to hash table." << endl;
    else {
      *entry = data;
      CurrentMemory += sizeof(PointerArenaStruct) + nSize + sizeof(GuardBytes);
      if (CurrentMemory > PeakMemory)
        PeakMemory = CurrentMemory;
    }
  }

  return data;
}


void * PObject::MemoryCheckAllocate(size_t count,
                                      size_t size, const char * file, int line)
{
  size_t total = count*size;
  void * data = MemoryCheckAllocate(total, file, line, NULL);
  if (data != NULL)
    memset(data, 0, total);
  return data;
}


void * PObject::MemoryCheckReallocate(void * ptr,
                                     size_t nSize, const char * file, int line)
{
  if (PointerHashTable == NULL)
    return realloc(ptr, nSize);

  void ** entry = FindPointerInHashTable(ptr, ptr);
  if (entry == NULL) {
    PError << "Pointer @" << ptr << " invalid for reallocate." << endl;
    return realloc(ptr, nSize);
  }

  PointerArenaStruct * arena =
            (PointerArenaStruct *)realloc(((PointerArenaStruct *)ptr)-1,
                      sizeof(PointerArenaStruct) + nSize + sizeof(GuardBytes));
  if (arena == NULL) {
    PAssertAlways(POutOfMemory);
    return NULL;
  }

  CurrentMemory -= arena->size;
  CurrentMemory += nSize;
  if (CurrentMemory > PeakMemory)
    PeakMemory = CurrentMemory;

  char * data = (char *)&arena[1];
  memcpy(&data[nSize], GuardBytes, sizeof(GuardBytes));

  arena->size      = nSize;
  arena->fileName  = file;
  arena->line      = line;

  *entry = data;

  return data;
}


void PObject::MemoryCheckDeallocate(void * ptr, const char * className)
{
  if (PointerHashTable == NULL) {
    free(ptr);
    return;
  }

  if (ptr == NULL)
    return;

  CurrentObjects--;

  void ** entry = FindPointerInHashTable(ptr, ptr);
  if (entry == NULL) {
    PError << "Pointer @" << ptr << " invalid for deallocation." << endl;
    free(ptr);
    return;
  }

  *entry = NULL;

  PointerArenaStruct * arena = ((PointerArenaStruct *)ptr)-1;

  CurrentMemory -= sizeof(PointerArenaStruct) + arena->size + sizeof(GuardBytes);

  if (arena->className != NULL && strcmp(arena->className, className) != 0)
    PError << "Pointer @" << ptr << " allocated as '" << arena->className
           << "' and deallocated as '" << className << "'." << endl;

  if (memcmp(arena->guard, GuardBytes, sizeof(GuardBytes)) != 0)
    PError << "Pointer @" << ptr << " underrun." << endl;

  if (memcmp((char *)ptr+arena->size, GuardBytes, sizeof(GuardBytes)) != 0)
    PError << "Pointer @" << ptr << " overrun." << endl;

  memset(ptr, 0x55, arena->size);  // Make use of freed data noticable
  free(arena);
}


#endif


const char * PObject::GetClass(unsigned) const
{
  return Class();
}


BOOL PObject::IsClass(const char * clsName) const
{
  return strcmp(clsName, Class()) == 0;
}


BOOL PObject::IsDescendant(const char * clsName) const
{
  return strcmp(clsName, Class()) == 0;
}


PObject::Comparison PObject::CompareObjectMemoryDirect(const PObject&obj) const
{
  return (Comparison)memcmp(this, &obj, sizeof(PObject));
}


PObject * PObject::Clone() const
{
  PAssertAlways(PUnimplementedFunction);
  return NULL;
}


PObject::Comparison PObject::Compare(const PObject & obj) const
{
  return (Comparison)CompareObjectMemoryDirect(obj);
}


void PObject::PrintOn(ostream & strm) const
{
  strm << GetClass();
}


void PObject::ReadFrom(istream &)
{
}


PINDEX PObject::PreSerialise(PSerialiser &)
{
  return 0;
}


void PObject::Serialise(PSerialiser &)
{
}


void PObject::UnSerialise(PUnSerialiser &)
{
}


PINDEX PObject::HashFunction() const
{
  return 0;
}


///////////////////////////////////////////////////////////////////////////////
// Serialisation support

PSerialRegistration * PSerialRegistration::creatorHashTable[
                                           PSerialRegistration::HashTableSize];

PINDEX PSerialRegistration::HashFunction(const char * className)
{
  PINDEX hash = (BYTE)className[0];
  if (className[0] != '\0') {
    hash += (BYTE)className[1];
    if (className[1] != '\0')
      hash += (BYTE)className[2];
  }
  return hash%HashTableSize;
}


PSerialRegistration::PSerialRegistration(const char * clsNam,
                                                          CreatorFunction func)
  : className(clsNam), creator(func)
{
  clash = NULL;

  PINDEX hash = HashFunction(className);
  if (creatorHashTable[hash] != NULL)
    creatorHashTable[hash]->clash = this;
  creatorHashTable[hash] = this;
}


PSerialRegistration::CreatorFunction
                           PSerialRegistration::GetCreator(const char * clsNam)
{
  PINDEX hash = HashFunction(clsNam);
  PSerialRegistration * reg = creatorHashTable[hash];
  while (reg != NULL) {
    if (strcmp(reg->className, clsNam) == 0)
      return reg->creator;
  }
  return NULL;
}


PSerialiser::PSerialiser(ostream & strm)
  : stream(strm)
{
}


PSerialiser & PSerialiser::operator<<(PObject & obj)
{
  obj.Serialise(*this);
  return *this;
}


PTextSerialiser::PTextSerialiser(ostream & strm, PObject & data)
  : PSerialiser(strm)
{
  data.Serialise(*this);
}


PSerialiser & PTextSerialiser::operator<<(char)
  { return *this; }

PSerialiser & PTextSerialiser::operator<<(unsigned char)
  { return *this; }

PSerialiser & PTextSerialiser::operator<<(signed char)
  { return *this; }

PSerialiser & PTextSerialiser::operator<<(short)
  { return *this; }

PSerialiser & PTextSerialiser::operator<<(unsigned short)
  { return *this; }

PSerialiser & PTextSerialiser::operator<<(int)
  { return *this; }

PSerialiser & PTextSerialiser::operator<<(unsigned int)
  { return *this; }

PSerialiser & PTextSerialiser::operator<<(long)
  { return *this; }

PSerialiser & PTextSerialiser::operator<<(unsigned long)
  { return *this; }

PSerialiser & PTextSerialiser::operator<<(float)
  { return *this; }

PSerialiser & PTextSerialiser::operator<<(double)
  { return *this; }

PSerialiser & PTextSerialiser::operator<<(long double)
  { return *this; }

PSerialiser & PTextSerialiser::operator<<(const char *)
  { return *this; }

PSerialiser & PTextSerialiser::operator<<(const unsigned char *)
  { return *this; }

PSerialiser & PTextSerialiser::operator<<(const signed char *)
  { return *this; }

PSerialiser & PTextSerialiser::operator<<(PObject & obj)
  { return PSerialiser::operator<<(obj); }


PBinarySerialiser::PBinarySerialiser(ostream & strm, PObject & data)
  : PSerialiser(strm)
{
  classesUsed = new PSortedStringList;
  data.PreSerialise(*this);
  stream << *classesUsed;
  data.Serialise(*this);
}


PBinarySerialiser::~PBinarySerialiser()
{
  delete classesUsed;
}


PSerialiser & PBinarySerialiser::operator<<(char)
  { return *this; }

PSerialiser & PBinarySerialiser::operator<<(unsigned char)
  { return *this; }

PSerialiser & PBinarySerialiser::operator<<(signed char)
  { return *this; }

PSerialiser & PBinarySerialiser::operator<<(short)
  { return *this; }

PSerialiser & PBinarySerialiser::operator<<(unsigned short)
  { return *this; }

PSerialiser & PBinarySerialiser::operator<<(int)
  { return *this; }

PSerialiser & PBinarySerialiser::operator<<(unsigned int)
  { return *this; }

PSerialiser & PBinarySerialiser::operator<<(long)
  { return *this; }

PSerialiser & PBinarySerialiser::operator<<(unsigned long)
  { return *this; }

PSerialiser & PBinarySerialiser::operator<<(float)
  { return *this; }

PSerialiser & PBinarySerialiser::operator<<(double)
  { return *this; }

PSerialiser & PBinarySerialiser::operator<<(long double)
  { return *this; }

PSerialiser & PBinarySerialiser::operator<<(const char *)
  { return *this; }

PSerialiser & PBinarySerialiser::operator<<(const unsigned char *)
  { return *this; }

PSerialiser & PBinarySerialiser::operator<<(const signed char *)
  { return *this; }

PSerialiser & PBinarySerialiser::operator<<(PObject & obj)
  { return PSerialiser::operator<<(obj); }


PUnSerialiser::PUnSerialiser(istream & strm)
  : stream(strm)
{
}


PTextUnSerialiser::PTextUnSerialiser(istream & strm)
  : PUnSerialiser(strm)
{
}


PUnSerialiser & PTextUnSerialiser::operator>>(char &)
  { return *this; }

PUnSerialiser & PTextUnSerialiser::operator>>(unsigned char &)
  { return *this; }

PUnSerialiser & PTextUnSerialiser::operator>>(signed char &)
  { return *this; }

PUnSerialiser & PTextUnSerialiser::operator>>(short &)
  { return *this; }

PUnSerialiser & PTextUnSerialiser::operator>>(unsigned short &)
  { return *this; }

PUnSerialiser & PTextUnSerialiser::operator>>(int &)
  { return *this; }

PUnSerialiser & PTextUnSerialiser::operator>>(unsigned int &)
  { return *this; }

PUnSerialiser & PTextUnSerialiser::operator>>(long &)
  { return *this; }

PUnSerialiser & PTextUnSerialiser::operator>>(unsigned long &)
  { return *this; }

PUnSerialiser & PTextUnSerialiser::operator>>(float &)
  { return *this; }

PUnSerialiser & PTextUnSerialiser::operator>>(double &)
  { return *this; }

PUnSerialiser & PTextUnSerialiser::operator>>(long double &)
  { return *this; }

PUnSerialiser & PTextUnSerialiser::operator>>(char *)
  { return *this; }

PUnSerialiser & PTextUnSerialiser::operator>>(unsigned char *)
  { return *this; }

PUnSerialiser & PTextUnSerialiser::operator>>(signed char *)
  { return *this; }

PUnSerialiser & PTextUnSerialiser::operator>>(PObject &)
  { return *this; }


PBinaryUnSerialiser::PBinaryUnSerialiser(istream & strm)
  : PUnSerialiser(strm)
{
  classesUsed = new PStringArray;
}


PBinaryUnSerialiser::~PBinaryUnSerialiser()
{
  delete classesUsed;
}


PUnSerialiser & PBinaryUnSerialiser::operator>>(char &)
  { return *this; }

PUnSerialiser & PBinaryUnSerialiser::operator>>(unsigned char &)
  { return *this; }

PUnSerialiser & PBinaryUnSerialiser::operator>>(signed char &)
  { return *this; }

PUnSerialiser & PBinaryUnSerialiser::operator>>(short &)
  { return *this; }

PUnSerialiser & PBinaryUnSerialiser::operator>>(unsigned short &)
  { return *this; }

PUnSerialiser & PBinaryUnSerialiser::operator>>(int &)
  { return *this; }

PUnSerialiser & PBinaryUnSerialiser::operator>>(unsigned int &)
  { return *this; }

PUnSerialiser & PBinaryUnSerialiser::operator>>(long &)
  { return *this; }

PUnSerialiser & PBinaryUnSerialiser::operator>>(unsigned long &)
  { return *this; }

PUnSerialiser & PBinaryUnSerialiser::operator>>(float &)
  { return *this; }

PUnSerialiser & PBinaryUnSerialiser::operator>>(double &)
  { return *this; }

PUnSerialiser & PBinaryUnSerialiser::operator>>(long double &)
  { return *this; }

PUnSerialiser & PBinaryUnSerialiser::operator>>(char *)
  { return *this; }

PUnSerialiser & PBinaryUnSerialiser::operator>>(unsigned char *)
  { return *this; }

PUnSerialiser & PBinaryUnSerialiser::operator>>(signed char *)
  { return *this; }

PUnSerialiser & PBinaryUnSerialiser::operator>>(PObject &)
  { return *this; }


///////////////////////////////////////////////////////////////////////////////
// General reference counting support

PSmartPointer::PSmartPointer(const PSmartPointer & ptr)
{
  object = ptr.object;
  if (object != NULL)
    ++object->referenceCount;
}


PSmartPointer & PSmartPointer::operator=(const PSmartPointer & ptr)
{
  if (object == ptr.object)
    return *this;

  if (object != NULL && --object->referenceCount == 0)
    delete object;

  object = ptr.object;
  if (object != NULL)
    ++object->referenceCount;

  return *this;
}


PSmartPointer::~PSmartPointer()
{
  if (object != NULL && --object->referenceCount == 0)
    delete object;
}


PObject::Comparison PSmartPointer::Compare(const PObject & obj) const
{
  PSmartObject * other = ((const PSmartPointer &)obj).object;
  if (object == other)
    return EqualTo;
  return object < other ? LessThan : GreaterThan;
}


///////////////////////////////////////////////////////////////////////////////
// Large integer support

#ifndef P_HAS_INT64

void PInt64__::Add(const PInt64__ & v)
{
  WORD * a = (WORD *)&low;
  WORD * b = (WORD *)&v.low;
  DWORD s = 0;
  for (int i = 0; i < 4; i++) {
    s = (DWORD)*a + (DWORD)*b + (s >> 16);
    *a = (WORD)s;
    a++;
    b++;
  }
}


void PInt64__::Sub(const PInt64__ & v)
{
  WORD * a = (WORD *)&low;
  WORD * b = (WORD *)&v.low;
  DWORD s = 0x10000;
  for (int i = 0; i < 4; i++) {
    s = (DWORD)*a - (DWORD)*b - (s >> 16) + 1;
    *a = (WORD)s;
    a++;
    b++;
  }
}


void PInt64__::Mul(const PInt64__ & v)
{
  WORD * a = (WORD *)&low;
  WORD * b = (WORD *)&v.low;
  DWORD p = 0;
  for (int i = 0; i < 4; i++) {
    p = (DWORD)*a * (DWORD)*b + (p >> 16);
    *a = (WORD)p;
    a++;
    b++;
  }
}


void PInt64__::Div(const PInt64__ & v)
{
}


void PInt64__::Mod(const PInt64__ & v)
{
}


BOOL PInt64::Lt(const PInt64 & v) const
{
  if ((long)high < (long)v.high)
    return TRUE;
  if ((long)high > (long)v.high)
    return FALSE;
  if ((long)high < 0)
    return (long)low > (long)v.low;
  return (long)low < (long)v.low;
}


BOOL PInt64::Gt(const PInt64 & v) const
{
  if ((long)high > (long)v.high)
    return TRUE;
  if ((long)high < (long)v.high)
    return FALSE;
  if ((long)high < 0)
    return (long)low < (long)v.low;
  return (long)low > (long)v.low;
}


BOOL PUInt64::Lt(const PUInt64 & v) const
{
  if (high < v.high)
    return TRUE;
  if (high > v.high)
    return FALSE;
  return low < high;
}


BOOL PUInt64::Gt(const PUInt64 & v) const
{
  if (high > v.high)
    return TRUE;
  if (high < v.high)
    return FALSE;
  return low > high;
}


static void Out64(ostream & stream, PUInt64 num)
{
  int base;
  switch (stream.flags()&ios::basefield) {
    case ios::oct :
      base = 8;
      break;
    case ios::hex :
      base = 16;
      break;
    default :
      base = 10;
  }

  char buf[25];
  char * p = &buf[sizeof(buf)];
  *--p = '\0';

  while (num != 0) {
    *--p = num%base + '0';
    if (*p > '9')
      *p += 7;
    num /= base;
  }

  if (*p == '\0')
    *--p = '0';

  stream << p;
}


ostream & operator<<(ostream & stream, const PInt64 & v)
{
  if (v >= 0)
    Out64(stream, v);
  else {
    int w = stream.width();
    stream.put('-');
    if (w > 0)
      stream.width(w-1);
    Out64(stream, -v);
  }

  return stream;
}


ostream & operator<<(ostream & stream, const PUInt64 & v)
{
  Out64(stream, v);
  return stream;
}


static PUInt64 Inp64(istream & stream)
{
  int base;
  switch (stream.flags()&ios::basefield) {
    case ios::oct :
      base = 8;
      break;
    case ios::hex :
      base = 16;
      break;
    default :
      base = 10;
  }

  if (isspace(stream.peek()))
    stream.get();

  PInt64 num = 0;
  while (isxdigit(stream.peek())) {
    int c = stream.get() - '0';
    if (c > 9)
      c -= 7;
    if (c > 9)
      c -= 32;
    num = num*base + c;
  }

  return num;
}


istream & operator>>(istream & stream, PInt64 & v)
{
  if (isspace(stream.peek()))
    stream.get();

  switch (stream.peek()) {
    case '-' :
      stream.ignore();
      {
        PInt64 i = Inp64(stream)
        v = -i;
      }
      break;
    case '+' :
      stream.ignore();
    default :
      v = Inp64(stream);
  }

  return stream;
}


istream & operator>>(istream & stream, PUInt64 & v)
{
  v = Inp64(stream);
  return stream;
}


#endif


// End Of File ///////////////////////////////////////////////////////////////
