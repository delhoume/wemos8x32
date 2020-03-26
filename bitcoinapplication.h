#if !defined(BITCOIN_APPLICATION)
#define BITCOIN_APPLICATION

#include "application.h"

class BitcoinApplication : public Application {
  public:
    BitcoinApplication(GFXcanvas& matrix);
    ~BitcoinApplication() {}
    void display();
};

#endif
