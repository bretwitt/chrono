%{
#include "chrono/physics/ChLinkTSDA.h"

using namespace chrono;

#ifdef SWIGCSHARP  // --------------------------------------------------------------------- CSHARP

// NESTED CLASSES: inherit stubs (not virtual) as outside classes

class ChForceFunctorP : public chrono::ChLinkTSDA::ForceFunctor {
    public:
        ChForceFunctorP() {}
        virtual double evaluate(double time,
                                double rest_length,
                                double length,
                                double vel,
                                chrono::ChLinkTSDA* link) override {
            GetLog() << "You must implement the function evaluate()!\n";
            return 0.0;
        }
};

#endif             // --------------------------------------------------------------------- CSHARP

%}

%shared_ptr(chrono::ChLinkTSDA)
%shared_ptr(chrono::ChLinkTSDA::ForceFunctor)

#ifdef SWIGCSHARP
%feature("director") ChForceFunctorP;
#endif

#ifdef SWIGPYTHON
%feature("director") ForceFunctor;
#endif
 
// Tell SWIG about parent class in Python
%import "ChLink.i"

#ifdef SWIGCSHARP  // --------------------------------------------------------------------- CSHARP

// NESTED CLASSES

class ChForceFunctorP {
    public:
        virtual ~ChForceFunctorP() {}
        virtual double evaluate(double time,
                                double rest_length,
                                double length,
                                double vel,
                                chrono::ChLinkTSDA* link) {
            return 0.0;
        }
};

%extend chrono::ChLinkTSDA
{
    void RegisterForceFunctor(std::shared_ptr<::ChForceFunctorP> functor) {
       $self->RegisterForceFunctor(functor);
    }
}

%ignore chrono::ChLinkTSDA::RegisterForceFunctor();

#endif             // --------------------------------------------------------------------- CSHARP

// Parse the header file to generate wrappers
%include "../../../chrono/physics/ChLinkTSDA.h"