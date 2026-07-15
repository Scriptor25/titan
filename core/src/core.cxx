#include <titan/component.hxx>
#include <titan/core.hxx>
#include <titan/log.hxx>
#include <titan/utils.hxx>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

titan::Application::Application(ApplicationInfo info)
    : m_Info(std::move(info)),
      m_Resources(*this),
      m_Entities(*this),
      m_Graphics(*this, m_Heap),
      m_Inputs(*this, m_Heap)
{
}

toolkit::result<> titan::Application::Initialize(const std::string_view exec, const std::vector<std::string_view> &args)
{
    (void) exec;
    (void) args;

    if (auto res = sequence(
        this,
        &Application::OnInitialize,
        &Application::InitializeWindow,
        &Application::InitializeAudio,
        &Application::InitializeXr); !res)
        return res;

    if (auto res = m_Graphics.Initialize(); !res)
        return res;

    if (auto res = m_Inputs.Initialize(); !res)
        return res;

    return OnStart();
}

void titan::Application::Terminate() const
{
    if (m_Window)
        m_Window.Close();
}

toolkit::result<bool> titan::Application::Spin()
{
    glfw::PollEvents();

    if (auto res = PollEvents(); !res || !*res)
        return res;
    if (auto res = RenderFrame(); !res)
        return res;

    return !m_Window.ShouldClose();
}

toolkit::result<> titan::Application::Destroy()
{
    if (auto res = OnStop(); !res)
        return res;

    if (auto res = m_Inputs.Destroy(); !res)
        return res;
    if (auto res = m_Entities.Destroy(); !res)
        return res;
    if (auto res = m_Graphics.Destroy(); !res)
        return res;
    if (auto res = m_Resources.Destroy(); !res)
        return res;

    return {};
}

const std::string &titan::Application::GetName() const
{
    return m_Info.Name;
}

titan::VersionInfo titan::Application::GetVersion() const
{
    return m_Info.Version;
}

titan::VersionInfo titan::Application::GetEngineVersion() const
{
    return {
        .Major = VERSION_MAJOR,
        .Minor = VERSION_MINOR,
        .Patch = VERSION_PATCH,
    };
}

titan::ResourceSystem &titan::Application::GetResources()
{
    return m_Resources;
}

titan::EntitySystem &titan::Application::GetEntities()
{
    return m_Entities;
}

titan::InputSystem &titan::Application::GetInputs()
{
    return m_Inputs;
}

titan::GraphicsSystem &titan::Application::GetGraphics()
{
    return m_Graphics;
}

titan::glfw::Window &titan::Application::GetWindow()
{
    return m_Window;
}

titan::xr::Instance &titan::Application::GetXrInstance()
{
    return m_XrInstance;
}

XrSystemId titan::Application::GetXrSystemId()
{
    return m_XrSystemId;
}

XrViewConfigurationType titan::Application::GetXrViewConfigurationType()
{
    return m_XrViewConfigurationType;
}

std::span<XrViewConfigurationView> titan::Application::GetXrViewConfigurationViews()
{
    return m_XrViewConfigurationViews;
}

titan::xr::Session &titan::Application::GetXrSession()
{
    return m_Graphics.m_Session;
}

toolkit::result<> titan::Application::InitializeXr()
{
    return sequence(
        this,
        &Application::InitializeXrInstance,
        &Application::InitializeXrMessenger,
        &Application::InitializeXrSystemId,
        &Application::InitializeXrViewConfigurationType,
        &Application::InitializeXrViewConfigurationViews,
        &Application::InitializeXrEnvironmentBlendMode);
}

toolkit::result<bool> titan::Application::PollEvents()
{
    XrEventDataBuffer event_data{ .type = XR_TYPE_EVENT_DATA_BUFFER };

    while (xrPollEvent(m_XrInstance, &event_data) == XR_SUCCESS)
    {
        switch (event_data.type)
        {
        case XR_TYPE_EVENT_DATA_EVENTS_LOST:
        {
            const auto events_lost = reinterpret_cast<XrEventDataEventsLost *>(&event_data);
            info("XrEventDataEventsLost {{ lostEventCount={} }}", events_lost->lostEventCount);
            break;
        }

        case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
        {
            const auto instance_loss_pending = reinterpret_cast<XrEventDataInstanceLossPending *>(&event_data);
            info("XrEventDataInstanceLossPending {{ lossTime={} }}", instance_loss_pending->lossTime);

            return false;
        }

        case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
        {
            const auto interaction_profile_changed = reinterpret_cast<XrEventDataInteractionProfileChanged *>(
                &event_data);
            info(
                "XrEventDataInteractionProfileChanged {{ session={} }}",
                static_cast<void *>(interaction_profile_changed->session));

            if (interaction_profile_changed->session != m_Graphics.m_Session)
            {
                info("XrEventDataInteractionProfileChanged for foreign session!");
                break;
            }

            if (auto res = m_Inputs.RecordBindings(); !res)
                return res;

            break;
        }

        case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
        {
            const auto reference_space_change_pending = reinterpret_cast<XrEventDataReferenceSpaceChangePending *>(
                &event_data);
            info(
                "XrEventDataReferenceSpaceChangePending {{ session={}, referenceSpaceType={}, changeTime={}, poseValid={} }}",
                static_cast<void *>(reference_space_change_pending->session),
                reference_space_change_pending->referenceSpaceType,
                reference_space_change_pending->changeTime,
                reference_space_change_pending->poseValid);

            if (reference_space_change_pending->session != m_Graphics.m_Session)
            {
                info("XrEventDataReferenceSpaceChangePending for foreign session!");
                break;
            }

            break;
        }

        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
        {
            const auto session_state_changed = reinterpret_cast<XrEventDataSessionStateChanged *>(&event_data);
            info(
                "XrEventDataSessionStateChanged {{ session={}, state={}, time={} }}",
                static_cast<void *>(session_state_changed->session),
                session_state_changed->state,
                session_state_changed->time);

            if (session_state_changed->session != m_Graphics.m_Session)
            {
                info("XrEventDataSessionStateChanged for foreign session!");
                break;
            }

            m_XrSessionState = session_state_changed->state;

            switch (session_state_changed->state)
            {
            case XR_SESSION_STATE_READY:
            {
                const XrSessionBeginInfo session_begin_info
                {
                    .type = XR_TYPE_SESSION_BEGIN_INFO,
                    .primaryViewConfigurationType = m_XrViewConfigurationType,
                };

                if (auto res = xrBeginSession(m_Graphics.m_Session, &session_begin_info))
                    return toolkit::make_error("xrBeginSession => {}", res);

                break;
            }

            case XR_SESSION_STATE_STOPPING:
            {
                if (auto res = xrEndSession(m_Graphics.m_Session))
                    return toolkit::make_error("xrEndSession => {}", res);

                info(
                    "XrEventDataSessionStateChanged {{ session={}, state={}, time={} }}",
                    static_cast<void *>(session_state_changed->session),
                    session_state_changed->state,
                    session_state_changed->time);
                return false;
            }

            case XR_SESSION_STATE_LOSS_PENDING:
            case XR_SESSION_STATE_EXITING:
            {
                info(
                    "XrEventDataSessionStateChanged {{ session={}, state={}, time={} }}",
                    static_cast<void *>(session_state_changed->session),
                    session_state_changed->state,
                    session_state_changed->time);
                return false;
            }

            default:
                break;
            }

            break;
        }

        default:
            break;
        }

        event_data.type = XR_TYPE_EVENT_DATA_BUFFER;
    }

    return true;
}

toolkit::result<> titan::Application::RenderFrame()
{
    const XrFrameWaitInfo frame_wait_info
    {
        .type = XR_TYPE_FRAME_WAIT_INFO,
    };

    XrFrameState frame_state;
    if (auto res = xr::WaitFrame(m_Graphics.m_Session, frame_wait_info) >> frame_state; !res)
        return res;

    if (auto res = PreFrame(); !res)
        return res;

    const XrFrameBeginInfo frame_begin_info
    {
        .type = XR_TYPE_FRAME_BEGIN_INFO,
    };

    if (auto res = xr::BeginFrame(m_Graphics.m_Session, frame_begin_info); !res)
        return res;

    if (auto res = OnFrame(); !res)
        return res;

    detail::LayerReference reference
    {
        .PredictedDisplayTime = frame_state.predictedDisplayTime,
    };

    const auto session_active = m_XrSessionState == XR_SESSION_STATE_SYNCHRONIZED
                                || m_XrSessionState == XR_SESSION_STATE_VISIBLE
                                || m_XrSessionState == XR_SESSION_STATE_FOCUSED;

    if (session_active && frame_state.shouldRender)
    {
        if (auto res = UpdateEntities(); !res)
            return res;

        if (auto res = UpdateInputs(frame_state.predictedDisplayTime); !res)
            return res;

        if (auto res = m_Graphics.RenderWindowView(); !res)
            return res;

        if (auto res = m_Graphics.RenderViews(reference); !res)
            return res;

        reference.Layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader *>(&reference.Projection));
    }

    const XrFrameEndInfo frame_end_info
    {
        .type = XR_TYPE_FRAME_END_INFO,
        .displayTime = frame_state.predictedDisplayTime,
        .environmentBlendMode = m_XrEnvironmentBlendMode,
        .layerCount = static_cast<uint32_t>(reference.Layers.size()),
        .layers = reference.Layers.data(),
    };

    if (auto res = xr::EndFrame(m_Graphics.m_Session, frame_end_info); !res)
        return res;

    return PostFrame();
}

toolkit::result<> titan::Application::UpdateEntities()
{
    for (auto [state, transform] : m_Entities.Query<component::Transform>())
    {
        if (!state.Active || !transform.Dirty)
            continue;

        transform.Dirty = false;

        const auto translation = glm::translate(glm::mat4(1.0f), transform.Translation);
        const auto rotation = glm::mat4_cast(transform.Rotation);
        const auto scale = glm::scale(glm::mat4(1.0f), transform.Scale);
        const auto translation_local = glm::translate(glm::mat4(1.0f), -transform.Pivot);

        transform.Matrix = translation * rotation * scale * translation_local;
        transform.Inverse = glm::inverse(transform.Matrix);
    }

    for (auto [state, camera] : m_Entities.Query<component::FrustumCamera>())
    {
        if (!state.Active || !camera.Dirty)
            continue;

        camera.Dirty = true;

        camera.Matrix = glm::frustumRH_ZO(
            camera.Left,
            camera.Right,
            camera.Bottom,
            camera.Top,
            camera.Near,
            camera.Far);
        camera.Inverse = glm::inverse(camera.Matrix);
    }

    for (auto [state, camera] : m_Entities.Query<component::AngleFrustumCamera>())
    {
        if (!state.Active || !camera.Dirty)
            continue;

        camera.Dirty = true;

        const auto left = camera.Near * tanf(camera.FovLeft);
        const auto right = camera.Near * tanf(camera.FovRight);
        const auto bottom = camera.Near * tanf(camera.FovDown);
        const auto top = camera.Near * tanf(camera.FovUp);

        camera.Matrix = glm::frustumRH_ZO(left, right, bottom, top, camera.Near, camera.Far);
        camera.Inverse = glm::inverse(camera.Matrix);
    }

    int width, height;
    m_Window.GetFramebufferSize(width, height);

    const auto aspect = static_cast<float>(width) / static_cast<float>(height);

    for (auto [state, camera] : m_Entities.Query<component::PerspectiveCamera>())
    {
        if (!state.Active || !camera.Dirty)
            continue;

        camera.Dirty = true;

        camera.Matrix = glm::perspectiveRH_ZO(camera.FovY, aspect, camera.Near, camera.Far);
        camera.Inverse = glm::inverse(camera.Matrix);
    }

    for (auto [state, script] : m_Entities.Query<component::Script>())
    {
        if (!script.Callee)
            continue;

        script.Callee(*this, { state.ID, state.Active });
    }

    return {};
}

toolkit::result<> titan::Application::UpdateInputs(XrTime time)
{
    return m_Inputs.Update(
        {
            .ViewSpace = m_Graphics.m_ViewSpace,
            .ReferenceSpace = m_Graphics.m_ReferenceSpace,
            .SessionState = m_XrSessionState,
            .Time = time,
        });
}

toolkit::result<> titan::Application::OnInitialize()
{
    return {};
}

toolkit::result<> titan::Application::OnStart()
{
    return {};
}

toolkit::result<> titan::Application::PreFrame()
{
    return {};
}

toolkit::result<> titan::Application::OnFrame()
{
    return {};
}

toolkit::result<> titan::Application::PostFrame()
{
    return {};
}

toolkit::result<> titan::Application::OnStop()
{
    return {};
}
