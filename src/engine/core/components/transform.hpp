#pragma once

#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>

#include "core/ecs.hpp"

// Source: BEE (Breda Educational Engine)

namespace niji
{
/// <summary>
/// Transform component. Contains the position, rotation and scale of the entity.
/// Implemented on top of the entity-component-system (entt).
/// </summary>
struct Transform
{
    /// <summary>
    /// The name of the entity. Used for debugging and editor purposes.
    /// If small, it can benefit from the small string optimization.
    /// </summary>
    std::string Name = {};

    /// <summary>
    /// Creates a Transform with default translation (0,0,0), scale (1,1,1), and rotation (identity
    /// quaternion).
    /// </summary>
    Transform()
    {
    }

    /// <summary>
    /// Creates a Transform with the given translation, scale, and rotation.
    /// </summary>
    Transform(const glm::vec3& translation, const glm::vec3& scale, const glm::quat& rotation)
        : m_translation(translation), m_scale(scale), m_rotation(rotation)
    {
    }

    /// <summary>
    /// Sets the parent of the entity. This will also add the entity to that parent's children.
    /// If this entity already had a parent before, this function will remove the child from that
    /// previous parent.
    /// </summary>
    /// <remarks>
    /// This function does not change the Transform's translation/rotation/scale.
    /// Changing the parent may move/rotate/scale this entity in world space.
    /// If that is undesired, the caller of this function should update the
    /// translation/rotation/scale themselves.
    /// </remarks>
    /// <param name="parent">The parent entity. Use entt::null to detach this entity from its
    /// current parent.</param>
    void SetParent(Entity parent);

    /// <summary>Returns whether or not the entity has any children.</summary>
    [[nodiscard]] bool HasChildren() const
    {
        return m_first != entt::null;
    }

    /// <summary>Returns whether or not the entity has a parent.</summary>
    [[nodiscard]] bool HasParent() const
    {
        return m_parent != entt::null;
    }

    /// <summary>Gets this Transform's translation in local space.</summary>
    [[nodiscard]] inline const glm::vec3& GetTranslation() const
    {
        return m_translation;
    }
    /// <summary>Gets this Transform's scale in local space.</summary>
    [[nodiscard]] inline const glm::vec3& GetScale() const
    {
        return m_scale;
    }
    /// <summary>Gets this Transform's rotation in local space.</summary>
    [[nodiscard]] inline const glm::quat& GetRotation() const
    {
        return m_rotation;
    }

    /// <summary>Gets the matrix that transforms from local space to world space.
    /// Calling this function recomputes the matrix when necessary.</summary>
    [[nodiscard]] const glm::mat4& World();

    /// <summary>Updates the translation of this Transform.
    /// Also marks this Transform and its children as dirty.</summary>
    /// <param name="translation">The new translation vector to use.</param>
    void SetTranslation(const glm::vec3& translation)
    {
        m_translation = translation;
        SetMatrixDirty();
    }

    /// <summary>Updates the scale of this Transform.
    /// Also marks this Transform and its children as dirty.</summary>
    /// <param name="scale">The new scale vector to use.</param>
    void SetScale(const glm::vec3& scale)
    {
        m_scale = scale;
        SetMatrixDirty();
    }

    /// <summary>Updates the rotation of this Transform.
    /// Also marks this Transform and its children as dirty.</summary>
    /// <param name="rotation">The new rotation quaternion to use.</param>
    void SetRotation(const glm::quat& rotation)
    {
        m_rotation = rotation;
        SetMatrixDirty();
    }

    /// Decomposes a transformation matrix into its translation, scale and rotation components,
    /// and stores the result in this Transform.
    void SetFromMatrix(const glm::mat4& transform);

  private:
    glm::vec3 m_translation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::quat m_rotation = glm::identity<glm::quat>();

    glm::mat4 m_worldMatrix = glm::identity<glm::mat4>();
    bool m_worldMatrixDirty = true;

    // The hierarchy is implemented as a linked list.
    Entity m_parent{entt::null};
    Entity m_first{entt::null};
    Entity m_next{entt::null};

    // Add a child to the entity. Called by SetParent().
    void AddChild(Entity child);
    // Removes a child from the entity. May be called by SetParent().
    void RemoveChild(Entity child);

    /// <summary>Recursively marks this transform and its children as dirty.
    /// This will cause the world matrix to be recomputed the next time you call World().</summary>
    void SetMatrixDirty();

  public:
    /// <summary>
    /// Iterator for the children of the entity.
    /// </summary>
    class Iterator
    {
      public:
        Iterator() = default;
        Iterator(Entity ent);
        Iterator& operator++();
        bool operator!=(const Iterator& iterator);
        Entity operator*();

      private:
        Entity m_current{entt::null};
    };

    /// <summary>
    /// The begin iterator for the children of the entity.
    /// </summary>
    Iterator begin()
    {
        return Iterator(m_first);
    }

    /// <summary>
    /// The end iterator for the children of the entity.
    /// </summary>
    Iterator end()
    {
        return Iterator();
    }
};

} // namespace niji