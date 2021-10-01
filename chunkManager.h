#pragma once

#include "EntityID.h"
#include "SigArch.h"
#include "tempPosition.h"

class ChunkManager
{
public:
    virtual EntityID allocateNewEntity(SigArch sa) = 0;
    virtual void deallocateEntity(EntityID id) = 0;
    virtual TempPosition getEntityPosition(EntityID id) = 0;
    virtual void setEntityPosition(EntityID id, TempPosition pos) = 0;
};
