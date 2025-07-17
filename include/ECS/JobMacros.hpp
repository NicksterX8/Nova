#include "Job.hpp"

/*
struct ChunkMap;

#define NAMESPACED_COMPONENT_ID_SIGNATURE(component, namespace) ECS::Signature{namespace::component::ID}

#define GET_COMPONENT_SIGNATURE(namespace, ...) MAP_LIST_UD_OR(NAMESPACED_COMPONENT_ID_SIGNATURE, namespace, __VA_ARGS__)


#define DECL_COMPONENT_ARRAY(component) EC::component* component = jobData->getComponentArray<EC::component>();

// #define DECL_COMPONENT_ARRAY(component) EC::component* COMBINE(component, Array) = jobData->getComponentArray<EC::component>();
// #define DECL_COMPONENT_REF_FROM_ARRAY(component) EC::component& component = COMBINE(component, Array)[N];

// #define EXECUTE_START_FUNC(job, ...) \
//     { \
//         using namespace EC; \
//         job.readComponents = ECS::getSignature<__VA_ARGS__>(); \
//         job.writeComponents = ECS::getSignature<__VA_ARGS__>(); \
//     } \
//     job.executeFunc = [](void* jobDataPtr, int _startIndex, int _endIndex){\
//     JobDataNew* jobData = (JobDataNew*)jobDataPtr; \
//     GroupVars& groupVars = *(GroupVars*)jobData->groupVars; \
//     Dependencies& deps = *(Dependencies*)jobData->dependencies; \
//     MAP(DECL_COMPONENT_ARRAY2, __VA_ARGS__) \
//     for (int N = _startIndex; N < _endIndex; N++) {
// #define EXECUTE_END_FUNC } };

#define EXECUTE_START_OBJECT_CLASS(job, ...) \
    { \
        using namespace EC; \
        job.readComponents = ECS::getSignature<__VA_ARGS__>(); \
        job.writeComponents = ECS::getSignature<__VA_ARGS__>(); \
    } \
    { \
    using ThisClass = std::remove_reference_t<decltype(*this)>; \
    job.dependencies = this; \
    job.executeFunc = [](void* jobDataPtr, int _startIndex, int _endIndex){\
    JobDataNew* jobData = (JobDataNew*)jobDataPtr; \
    GroupVars& groupVars = *(GroupVars*)jobData->groupVars; \
    ThisClass& self = *(ThisClass*)jobData->dependencies; \
    MAP(DECL_COMPONENT_ARRAY2, __VA_ARGS__) \
    for (int N = _startIndex; N < _endIndex; N++) {
#define EXECUTE_END_OBJECT_CLASS } }; }

// #define EXECUTE_START_CLASS4 \
//     { \
//     using ThisClass = std::remove_reference_t<decltype(*this)>; \
//     this->dependencies = this; \
//     this->executeFunc = [](void* jobDataPtr, int _startIndex, int _endIndex){\
//     JobDataNew* jobData = (JobDataNew*)jobDataPtr; \
//     GroupVars& groupVars = *(GroupVars*)jobData->groupVars; \
//     ThisClass& self = *(ThisClass*)jobData->dependencies; \
//     for (int N = _startIndex; N < _endIndex; N++) {
// #define EXECUTE_END_CLASS4 } }; }

#define EXECUTE_START_CLASS(...) \
    { \
        using namespace EC; \
        this->readComponents = ECS::getSignature<__VA_ARGS__>(); \
        this->writeComponents = ECS::getSignature<__VA_ARGS__>(); \
    } \
    { \
    using ThisClass = std::remove_reference_t<decltype(*this)>; \
    this->dependencies = this; \
    this->executeFunc = [](void* jobDataPtr, int _startIndex, int _endIndex){\
    JobDataNew* jobData = (JobDataNew*)jobDataPtr; \
    GroupVars& groupVars = *(GroupVars*)jobData->groupVars; \
    ThisClass& self = *(ThisClass*)jobData->dependencies; \
    MAP(DECL_COMPONENT_ARRAY, __VA_ARGS__) \
    for (int N = _startIndex; N < _endIndex; N++) {
#define EXECUTE_END_CLASS } }; }

#define USE_GROUP_VARS(groupvars) using GroupVars = groupvars;

// params: components needed to run
#define EXECUTE_IF_START(...) { \
        using namespace EC; \
        this->readComponents |= ECS::getSignature<__VA_ARGS__>(); \
        this->writeComponents |= ECS::getSignature<__VA_ARGS__>(); \
    } \
    { \
    using ThisClass = std::remove_reference_t<decltype(*this)>; \
    this->addConditionalExecute(GET_COMPONENT_SIGNATURE(EC, __VA_ARGS__), [](void* jobDataPtr, int _startIndex, int _endIndex){\
    JobDataNew* jobData = (JobDataNew*)jobDataPtr; \
    GroupVars& groupVars = *(GroupVars*)jobData->groupVars; \
    ThisClass& self = *(ThisClass*)jobData->dependencies; \
    MAP(DECL_COMPONENT_ARRAY, __VA_ARGS__) \
    for (int N = _startIndex; N < _endIndex; N++) {
        
#define EXECUTE_IF_END } }); }

struct PositionsArray {
    GroupArray<int> positions;
};

struct MoveJob2 : Job {
    int scale = -1;
    ChunkMap* chunkmap;

    MoveJob2(ChunkMap* chunkmap) : chunkmap(chunkmap) {
        USE_GROUP_VARS(PositionsArray);
        EXECUTE_START_CLASS(ViewBox)
            groupVars.positions[N] = ViewBox[N].box.min.x * self.scale;
        EXECUTE_END_CLASS
        SetConst<EC::ViewBox>();
    }

    void update(int scale) {
        this->scale = scale;
    }
};

struct MoveJob3 : Job {
    int scale = -1;
    ChunkMap* chunkmap;

    USE_GROUP_VARS(PositionsArray);

    MoveJob3(ChunkMap* chunkmap) : chunkmap(chunkmap) {
        EXECUTE_START_CLASS(ViewBox)
            groupVars.positions[N] = ViewBox[N].box.min.x * self.scale;
        EXECUTE_END_CLASS
    }

    void update(int scale) {
        this->scale = scale;
    }
};
*/