#include "QueryStore.h"

namespace ad {
namespace ent {


QueryStore::QueryStore(const QueryStore & aRhs)
{
    for(const auto & [key, storagePtr] : aRhs)
    {
        emplace(key, storagePtr->clone());
    }
}


QueryStore & QueryStore::operator=(const QueryStore & aRhs)
{
    QueryStore copy{aRhs};
    swap(copy);
    return *this;
}


void QueryStore::swap(QueryStore & aRhs)
{
    std::swap(static_cast<Parent_t &>(*this),
              static_cast<Parent_t &>(aRhs));
}


} // namespace ent
} // namespace ad
