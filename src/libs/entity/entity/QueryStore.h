#pragma once


#include "Component.h"
#include "detail/QueryBackend.h"

#include <map>
#include <memory>

namespace ad {
namespace ent {


// TODO Ad 2022/07/08: Replace the TypeSequence by a TypeSet.
// This will imply that all queries on the same set of components,
// **independently of the order**, will share the same QueryBackend.
// The main complication will be that the Query will need to statically sort the components
// to instantiate their pointer to backend.
class QueryStore : public std::map<TypeSequence, std::unique_ptr<detail::QueryBackendBase>>
{
    using Parent_t = std::map<TypeSequence, std::unique_ptr<detail::QueryBackendBase>>;

public:
    QueryStore() = default;
    ~QueryStore() = default;

    QueryStore(const QueryStore & aRhs);
    QueryStore & operator=(const QueryStore & aRhs);

    QueryStore(QueryStore && aRhs) = default;
    QueryStore & operator=(QueryStore && aRhs) = default;

private:
    void swap(QueryStore & aRhs);
};

} // namespace ent
} // namespace ad
