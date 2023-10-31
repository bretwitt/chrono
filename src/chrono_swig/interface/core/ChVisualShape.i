%{
#include "chrono/assets/ChVisualShape.h"
#include "chrono/assets/ChVisualShapeFEA.h"
#include "chrono/assets/ChVisualShapeBox.h"
#include "chrono/assets/ChVisualShapeModelFile.h"
#include "chrono/assets/ChVisualShapeSphere.h"
#include "chrono/assets/ChVisualShapeCylinder.h"

using namespace chrono;
%}

namespace chrono {
class ChPhysicsItem;
namespace fea {
class ChMesh;
class ChNodeFEAxyz;
class ChNodeFEAxyzP;
class ChElementBase;
}}

%template(material_list) std::vector<std::shared_ptr<chrono::ChVisualMaterial>>;

%shared_ptr(chrono::ChVisualShape)
%shared_ptr(chrono::ChVisualShapeFEA)

%include "../../../chrono/assets/ChVisualShape.h"    
%include "../../../chrono/assets/ChVisualShapeFEA.h"    
%include "../../../chrono/assets/ChVisualShapeBox.h"    
%include "../../../chrono/assets/ChVisualShapeModelFile.h"  
%include "../../../chrono/assets/ChVisualShapeSphere.h" 
%include "../../../chrono/assets/ChVisualShapeCylinder.h"  
