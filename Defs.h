//
// Defs.h
//
#ifndef __H_Defs__
#define __H_Defs__

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }

#define SAFE_RELEASE(punk)  \
              if ((punk) != nullptr)  \
                { (punk)->Release(); (punk) = nullptr; }

#define SAFE_DELETE(pobj)  \
              if ((pobj) != nullptr)  \
                { delete (pobj); (pobj) = nullptr; }

#define SAFE_DELETE_ARRAY(pobj)  \
              if ((pobj) != nullptr)  \
                { delete[] (pobj); (pobj) = nullptr; }

#endif // __H_Defs__