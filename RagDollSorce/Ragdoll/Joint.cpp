#include "Joint.h"
#include "PhysicsSystem.h"

Joint::~Joint()
{
    if (_registry != nullptr)
    {
        _registry->RemoveJoint(this);
    }
}
