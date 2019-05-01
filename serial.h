
#define HEX 1
#define BIN 2

void *begin(void *);
class CSerial
{
 public:
  int available();
  int print(long);
  int print(const char* str);
  int print(const char*,int);
  int println(const uint32_t&,int);
  int println(const char* str="");
  int read();
  size_t write(const int);
};

extern CSerial Serial;
