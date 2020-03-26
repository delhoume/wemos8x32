#if !defined(TPM2_APPLICATION)
#define TPM2_APPLICATION

#include "application.h"

class TPM2Application : public Application {
  public:
    TPM2Application(GFXcanvas& matrix);
    ~TPM2Application() {}
    void display();
    void init();
  private:
     boolean _init;
};

#endif
