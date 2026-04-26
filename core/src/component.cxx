#include <titan/component.hxx>

void titan::BaseComponent::Update(const Context &context)
{
}

void titan::TransformComponent::Update(const Context &context)
{
    const auto translation = glm::translate(glm::mat4(1.0f), Position);
    const auto rotation = glm::mat4_cast(Orientation);
    const auto scale = glm::scale(glm::mat4(1.0f), Scale);
    const auto translation_local = glm::translate(glm::mat4(1.0f), -Pivot);

    Matrix = translation * rotation * scale * translation_local;
    Inverse = glm::inverse(Matrix);
}

void titan::FrustumCameraComponent::Update(const Context &context)
{
    Matrix = glm::frustumRH_ZO(Left, Right, Bottom, Top, Near, Far);
    Inverse = glm::inverse(Matrix);
}

void titan::AngleFrustumCameraComponent::Update(const Context &context)
{
    const auto left = Near * tanf(FovLeft);
    const auto right = Near * tanf(FovRight);
    const auto bottom = Near * tanf(FovDown);
    const auto top = Near * tanf(FovUp);

    Matrix = glm::frustumRH_ZO(left, right, bottom, top, Near, Far);
    Inverse = glm::inverse(Matrix);
}

void titan::PerspectiveCameraComponent::Update(const Context &context)
{
    Matrix = glm::perspectiveRH_ZO(FovY, context.Aspect, Near, Far);
    Inverse = glm::inverse(Matrix);
}
