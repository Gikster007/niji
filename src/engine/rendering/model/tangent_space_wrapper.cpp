#include "tangent_space_wrapper.hpp"

bool MikkTSpaceTangent::GetTangents(const MikktSpaceMesh& mesh, std::vector<glm::vec4>& tangents)
{
    if (mesh.m_indices.empty() || mesh.m_positions.empty() || mesh.m_normals.empty() || mesh.m_texcoords.empty())
    {
        printf("One or more MikktSpaceMesh mesh pointers are null. \n");
        return false;
    }

    SMikkTSpaceInterface mikkTInterface = {};
    mikkTInterface.m_getNumFaces = GetNumFaces;
    mikkTInterface.m_getNumVerticesOfFace = GetNumVerticesOfFace;
    mikkTInterface.m_getPosition = GetPosition;
    mikkTInterface.m_getNormal = GetNormal;
    mikkTInterface.m_getTexCoord = GetTexCoord;
    mikkTInterface.m_setTSpaceBasic = SetTangent;

    MikkiTGltfContext context = {};
    context.m_outTangents.resize(mesh.m_positions.size());
    context.m_mesh = &mesh;

    SMikkTSpaceContext mikkContext = {};
    mikkContext.m_pInterface = &mikkTInterface;
    mikkContext.m_pUserData = &context;

    const bool succes = genTangSpaceDefault(&mikkContext);
    tangents = context.m_outTangents;

    if (!succes)
        printf("Error loading MikktSpaceMesh tangents. \n");
    else
        printf("Generated tangents using MikktSpace. \n");

    return succes;
}

int MikkTSpaceTangent::GetNumFaces(const SMikkTSpaceContext* context)
{
    auto c = reinterpret_cast<MikkiTGltfContext*>(context->m_pUserData);
    return static_cast<int>(c->m_mesh->m_indices.size() / 3);
}

int MikkTSpaceTangent::GetNumVerticesOfFace(const SMikkTSpaceContext*, const int) { return 3; }

void MikkTSpaceTangent::GetPosition(const SMikkTSpaceContext* context, float posOut[3], const int faceIdx, const int vertIdx)
{
    auto c = reinterpret_cast<MikkiTGltfContext*>(context->m_pUserData);
    const uint32_t index = c->m_mesh->m_indices[faceIdx * 3 + vertIdx];
    const glm::vec3& position = c->m_mesh->m_positions[index];
    posOut[0] = position.x;
    posOut[1] = position.y;
    posOut[2] = position.z;
}

void MikkTSpaceTangent::GetNormal(const SMikkTSpaceContext* context, float normOut[3], const int faceIdx, const int vertIdx)
{
    auto c = reinterpret_cast<MikkiTGltfContext*>(context->m_pUserData);
    const uint32_t index = c->m_mesh->m_indices.at(faceIdx * 3 + vertIdx);
    const glm::vec3& normal = c->m_mesh->m_normals.at(index);
    normOut[0] = normal.x;
    normOut[1] = normal.y;
    normOut[2] = normal.z;
}

void MikkTSpaceTangent::GetTexCoord(const SMikkTSpaceContext* context, float uvOut[2], const int faceIdx, const int vertIdx)
{
    auto c = reinterpret_cast<MikkiTGltfContext*>(context->m_pUserData);
    const uint32_t index = c->m_mesh->m_indices.at(faceIdx * 3 + vertIdx);
    const glm::vec2& uv = c->m_mesh->m_texcoords.at(index);
    uvOut[0] = uv.x;
    uvOut[1] = uv.y;
}

void MikkTSpaceTangent::SetTangent(const SMikkTSpaceContext* context,
                                   const float tangent[3],
                                   const float bitangentSign,
                                   const int faceIdx,
                                   const int vertIdx)
{
    auto c = reinterpret_cast<MikkiTGltfContext*>(context->m_pUserData);
    const uint32_t index = c->m_mesh->m_indices.at(faceIdx * 3 + vertIdx);
    c->m_outTangents[index] = glm::vec4(tangent[0], tangent[1], tangent[2], bitangentSign);
}