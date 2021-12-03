#pragma once
#include <iterator>
#include <cstddef>
#include <vector>
#include <algorithm>
#include "chunk.h"
#include "Types.h"
#include "ComponentManager.h"

// this class stores the result of a search
// it searches for which entities contain the specified components
// it can be used to access those components of those entities
class EntityQuery
{
    friend class EntityCoordinator;

private:
    // entity count
    int entityCount = 0;
    int chunkListVersion = -1;

    // chunks
    std::vector<Chunk*> chunks;

    // the components the query is searching for 
    std::vector<ComponentType> compTypes;

    // currently used for debugging
    std::vector<EntityID> entityIDs;

    // tags to search for
    std::vector<Tag> tags;

    void searchChunks(std::vector<Chunk*>& allChunks, int _chunkListVersion);
        
public:

    EntityQuery();
    int totalEntitiesFound();
    int chunkCount();
    int getChunkListVersion();
    Chunk* chunk(int i);
    //std::size_t ComponentTypesHash();
    //std::size_t TagsHash();
    //std::size_t QueryHash();

    void recountFoundEntities();

    // skip search if you already know the chunks you want
    EntityQuery(std::vector<Chunk*> chosenChunks);

    // entity queries perform their search when they are created
    //EntityQuery(std::vector<ComponentType> _compTypes, std::vector<Chunk*> allChunks, EntityQueryCache& cache);

    // entity queries perform their search when they are created
    EntityQuery(std::vector<ComponentType> _compTypes, std::vector<Tag> _tags);

    // get a vector of pointers for the components of the specified type
    // the components belonging to the entities found in the query
    template<typename T>
    std::vector<T*> getComponentArray()
    {
        //ComponentType type = ComponentManager::GetComponentType<T>();
        //if (std::find(compTypes.begin(),compTypes.end(),type) == compTypes.end())
        //{
        //    throw "cannot get component array from query: query does not contain this type";
        //}

        std::vector<T*> list;

        for (int i = 0; i < chunks.size(); i++)
        {
            int chunkEnts = chunks[i]->getCurrEntCount();
            T* compArray = chunks[i]->getComponentArray<T>();
            int c_index = 0;
            for (int j = 0; j < chunkEnts; j++, c_index++)
            {                
                int whileCounter = 0;
                while (!chunks[i]->isDataIndexActive(c_index))
                {
                    c_index++;
                    whileCounter++;
                    if (whileCounter > 16)
                    {
                        throw "loop limit reached";
                    }
                }
                EntityID id = chunks[i]->entityAtComponentIndex(c_index);
                entityIDs.push_back(id);
                list.push_back(compArray + c_index);
            }
        }

        return list;
    }

    std::vector<Chunk*> foundChunks();
    
    void DeleteFoundEntities();

    static std::size_t TagsHash(std::vector<Tag>& tags);
    static std::size_t ComponentTypesHash(std::vector<ComponentType>& compTypes);
    static std::size_t QueryParamterHash(std::vector<ComponentType>& compTypes, std::vector<Tag>& tags);
};
