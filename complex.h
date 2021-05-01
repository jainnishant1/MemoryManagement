#ifndef __COMPLEX_H__
#define __COMPLEX_H__

extern MemoryManager gMemoryManager;

class Complex 
  {
public:
  Complex() : r(0), c(0) {}
  Complex (double a, double b): r (a), c (b) {}
  // inline void* operator new(size_t size)
  //   { 
  //   return gMemoryManager.allocate(size);
  //   }

  // inline void  operator delete(void* object)
  //   {
  //   gMemoryManager.free(object);
  //   }

  // inline void* operator new [](size_t size)
  //   { 
  //   return gMemoryManager.allocate(size);
  //   }

  // inline void  operator delete [](void* object)
  //   {
  //   gMemoryManager.free(object);
  //   } 
  private:
    double r; // Real Part
    double c; // Complex Part
  };

#endif