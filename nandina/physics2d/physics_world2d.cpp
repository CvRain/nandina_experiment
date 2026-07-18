#include "physics_world2d.hpp"

#include "../scene/node2d.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace nandina::physics2d
{

    namespace
    {
        [[nodiscard]] auto to_b2_type(const PhysicsBodyType type) -> b2BodyType {
            switch (type) {
            case PhysicsBodyType::static_body: return b2_staticBody;
            case PhysicsBodyType::kinematic_body: return b2_kinematicBody;
            case PhysicsBodyType::dynamic_body: return b2_dynamicBody;
            }
            return b2_staticBody;
        }
    } // namespace

    PhysicsBody2D::PhysicsBody2D(
        PhysicsWorld2D& world,
        const b2BodyId id,
        const PhysicsBodyType type
    ): world_(&world), id_(id), type_(type) {}

    auto PhysicsBody2D::valid() const -> bool {
        return world_ != nullptr && B2_IS_NON_NULL(id_) && b2Body_IsValid(id_);
    }

    auto PhysicsBody2D::type() const -> PhysicsBodyType { return type_; }

    auto PhysicsBody2D::position() const -> foundation::NanPoint {
        if (!valid()) {
            throw std::logic_error("PhysicsBody2D is not valid");
        }
        return world_->from_b2(b2Body_GetPosition(id_));
    }

    void PhysicsBody2D::set_position(const foundation::NanPoint position) {
        if (!valid()) {
            throw std::logic_error("PhysicsBody2D is not valid");
        }
        b2Body_SetTransform(id_, world_->to_b2(position), b2Body_GetRotation(id_));
    }

    auto PhysicsBody2D::rotation() const -> float {
        if (!valid()) {
            throw std::logic_error("PhysicsBody2D is not valid");
        }
        return b2Rot_GetAngle(b2Body_GetRotation(id_));
    }

    void PhysicsBody2D::set_rotation(const float radians) {
        if (!valid()) {
            throw std::logic_error("PhysicsBody2D is not valid");
        }
        b2Body_SetTransform(id_, b2Body_GetPosition(id_), b2MakeRot(radians));
    }

    auto PhysicsBody2D::linear_velocity() const -> foundation::NanPoint {
        if (!valid()) {
            throw std::logic_error("PhysicsBody2D is not valid");
        }
        return world_->from_b2(b2Body_GetLinearVelocity(id_));
    }

    void PhysicsBody2D::set_linear_velocity(const foundation::NanPoint velocity) {
        if (!valid()) {
            throw std::logic_error("PhysicsBody2D is not valid");
        }
        b2Body_SetLinearVelocity(id_, world_->to_b2(velocity));
    }

    auto PhysicsBody2D::visual() const -> scene::NanNode2D* { return visual_.lock().get(); }
    void PhysicsBody2D::bind_visual(const std::shared_ptr<scene::NanNode2D>& visual) {
        visual_ = visual;
    }

    PhysicsWorld2D::PhysicsWorld2D(PhysicsWorldConfig config): config_(config) {
        if (!std::isfinite(config_.pixels_per_meter) || config_.pixels_per_meter <= 0.0F) {
            throw std::invalid_argument("pixels_per_meter must be finite and positive");
        }
        if (!std::isfinite(config_.fixed_timestep) || config_.fixed_timestep <= 0.0F) {
            throw std::invalid_argument("fixed_timestep must be finite and positive");
        }
        if (config_.substeps <= 0 || config_.max_steps_per_tick <= 0) {
            throw std::invalid_argument("physics step counts must be positive");
        }
        auto definition = b2DefaultWorldDef();
        definition.gravity = to_b2(config_.gravity);
        world_id_ = b2CreateWorld(&definition);
        if (B2_IS_NULL(world_id_)) {
            throw std::runtime_error("failed to create Box2D world");
        }
    }

    PhysicsWorld2D::~PhysicsWorld2D() { destroy_world(); }

    auto PhysicsWorld2D::create(PhysicsWorldConfig config) -> std::shared_ptr<PhysicsWorld2D> {
        return std::make_shared<PhysicsWorld2D>(config);
    }

    auto PhysicsWorld2D::config() const -> const PhysicsWorldConfig& { return config_; }
    auto PhysicsWorld2D::body_count() const -> std::size_t { return bodies_.size(); }
    auto PhysicsWorld2D::accumulator() const -> float { return accumulator_; }
    auto PhysicsWorld2D::total_steps() const -> std::size_t { return total_steps_; }

    auto PhysicsWorld2D::pixels_to_meters(const foundation::NanPoint value) const
        -> foundation::NanPoint {
        return value * (1.0F / config_.pixels_per_meter);
    }

    auto PhysicsWorld2D::meters_to_pixels(const foundation::NanPoint value) const
        -> foundation::NanPoint {
        return value * config_.pixels_per_meter;
    }

    auto PhysicsWorld2D::to_b2(const foundation::NanPoint value) const -> b2Vec2 {
        const auto meters = pixels_to_meters(value);
        return {.x = meters.get_x(), .y = meters.get_y()};
    }

    auto PhysicsWorld2D::from_b2(const b2Vec2 value) const -> foundation::NanPoint {
        return meters_to_pixels(foundation::NanPoint(value.x, value.y));
    }

    auto PhysicsWorld2D::create_body(PhysicsBodyDef def) -> std::shared_ptr<PhysicsBody2D> {
        if (!b2World_IsValid(world_id_)) {
            throw std::logic_error("PhysicsWorld2D is not valid");
        }
        auto definition = b2DefaultBodyDef();
        definition.type = to_b2_type(def.type);
        definition.position = to_b2(def.position);
        definition.rotation = b2MakeRot(def.rotation);
        definition.linearVelocity = to_b2(def.linear_velocity);
        definition.angularVelocity = def.angular_velocity;
        definition.fixedRotation = def.fixed_rotation;
        definition.isBullet = def.bullet;
        const auto id = b2CreateBody(world_id_, &definition);
        if (B2_IS_NULL(id)) {
            throw std::runtime_error("failed to create Box2D body");
        }
        auto body = std::shared_ptr<PhysicsBody2D>(new PhysicsBody2D(*this, id, def.type));
        body->bind_visual(def.visual);
        bodies_.push_back(body);
        return body;
    }

    void PhysicsWorld2D::destroy_body(PhysicsBody2D& body) {
        if (body.world_ != this) {
            return;
        }
        if (stepping_) {
            throw std::logic_error("cannot destroy a physics body while stepping");
        }
        if (body.valid()) {
            b2DestroyBody(body.id_);
        }
        body.id_ = b2_nullBodyId;
        body.world_ = nullptr;
        body.visual_.reset();
        std::erase_if(bodies_, [&body](const auto& owned) { return owned.get() == &body; });
    }

    void PhysicsWorld2D::sync_scene_to_physics() {
        for (const auto& body: bodies_) {
            auto visual = body->visual_.lock();
            if (!body->valid() || visual == nullptr || body->type_ == PhysicsBodyType::dynamic_body) {
                continue;
            }
            b2Body_SetTransform(
                body->id_, to_b2(visual->global_position()), b2MakeRot(visual->global_rotation())
            );
        }
    }

    void PhysicsWorld2D::sync_physics_to_scene() {
        for (const auto& body: bodies_) {
            auto visual = body->visual_.lock();
            if (!body->valid() || visual == nullptr || body->type_ != PhysicsBodyType::dynamic_body) {
                continue;
            }
            visual->set_global_position(from_b2(b2Body_GetPosition(body->id_)));
            const auto parent_rotation = visual->parent() != nullptr
                && visual->parent()->as_node2d() != nullptr
                ? visual->parent()->as_node2d()->global_rotation()
                : 0.0F;
            visual->set_rotation(b2Rot_GetAngle(b2Body_GetRotation(body->id_)) - parent_rotation);
        }
    }

    auto PhysicsWorld2D::step(const float dt) -> int {
        if (!std::isfinite(dt) || dt < 0.0F || !b2World_IsValid(world_id_)) {
            return 0;
        }
        accumulator_ += dt;
        int steps = 0;
        stepping_ = true;
        while (accumulator_ >= config_.fixed_timestep && steps < config_.max_steps_per_tick) {
            sync_scene_to_physics();
            b2World_Step(world_id_, config_.fixed_timestep, config_.substeps);
            sync_physics_to_scene();
            accumulator_ -= config_.fixed_timestep;
            ++steps;
            ++total_steps_;
        }
        stepping_ = false;
        if (steps == config_.max_steps_per_tick && accumulator_ >= config_.fixed_timestep) {
            accumulator_ = std::fmod(accumulator_, config_.fixed_timestep);
        }
        return steps;
    }

    void PhysicsWorld2D::physics_step(const float dt) {
        (void)step(dt);
    }

    void PhysicsWorld2D::destroy_world() {
        for (const auto& body: bodies_) {
            body->id_ = b2_nullBodyId;
            body->world_ = nullptr;
            body->visual_.reset();
        }
        bodies_.clear();
        if (B2_IS_NON_NULL(world_id_) && b2World_IsValid(world_id_)) {
            b2DestroyWorld(world_id_);
        }
        world_id_ = b2_nullWorldId;
        accumulator_ = 0.0F;
    }

} // namespace nandina::physics2d
