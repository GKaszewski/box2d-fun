#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <iostream>

const int WORLD_SCALE = 30;
const int PLAYER_FOOT_ID = 3;
const int GROUND_ID = 2;

sf::Vector2f convert_b2vector_to_sf_vector(b2Vec2 vec) {
    auto sf_vector = sf::Vector2f(vec.x * WORLD_SCALE, vec.y * WORLD_SCALE);
    return sf_vector;
}

b2Vec2 convert_sf_vector_to_b2vector(sf::Vector2f vec) {
    auto b2vector = b2Vec2(vec.x / WORLD_SCALE, vec.y / WORLD_SCALE);
    return b2vector;
}

class Shape {
public:
    Shape(float x, float y, sf::Texture& texture);

    void set_as_circle(float radius);
    void set_as_triangle(float base, float height);
    void set_as_rectangle(float width, float height, bool textured);

    virtual void create_body(b2World& world, bool dynamic = true);
    sf::Shape& get_shape() { return *shape; }
    b2Body& get_body() { return *body; }
    void update();
    int id;
    float fricition = 0.3f;

protected:
    sf::Vector2f position;
    sf::Shape* shape;
    b2Body* body;
    sf::Texture& texture;
};

Shape::Shape(float x, float y, sf::Texture & texture) : position(x,y), texture(texture) {
}

void Shape::set_as_circle(float radius) {
    sf::CircleShape* circle = new sf::CircleShape(radius);
    circle->setPosition(position);
    circle->setOrigin(radius, radius);
    circle->setTexture(&texture);
    circle->setTextureRect(sf::IntRect(112, 16, 16, 16));
    shape = circle;
}

void Shape::set_as_triangle(float base, float height) {
    sf::ConvexShape* triangle = new sf::ConvexShape(3);
    triangle->setPoint(0, sf::Vector2f(0, height));
    triangle->setPoint(1, sf::Vector2f(base / 2, 0));
    triangle->setPoint(2, sf::Vector2f(base, height));
    triangle->setPosition(position);
    triangle->setOrigin(base / 2, height / 2);
    triangle->setTexture(&texture);
    triangle->setTextureRect(sf::IntRect(112, 16, 16, 16));
    shape = triangle;
}

void Shape::set_as_rectangle(float width, float height, bool textured) {
    sf::RectangleShape* rectangle = new sf::RectangleShape(sf::Vector2f(width, height));
    rectangle->setPosition(position);
    rectangle->setOrigin(width / 2, height / 2);
    if (textured) {
        rectangle->setTexture(&texture);
        rectangle->setTextureRect(sf::IntRect(113, 16, 15, 16));
    }
    else {
        rectangle->setFillColor(sf::Color::Green);
    }
    
    shape = rectangle;
}

void Shape::create_body(b2World& world, bool dynamic) {
    b2BodyDef body_def;
    if (dynamic) body_def.type = b2_dynamicBody;
    auto vec = convert_sf_vector_to_b2vector(position);
    body_def.position.Set(vec.x, vec.y);
    body = world.CreateBody(&body_def);

    b2PolygonShape polygon_shape;
    polygon_shape.SetAsBox(shape->getLocalBounds().width / 2 / WORLD_SCALE,
        shape->getLocalBounds().height / 2 / WORLD_SCALE);

    b2FixtureDef fixture_def;
    fixture_def.shape = &polygon_shape;
    fixture_def.density = dynamic;
    fixture_def.friction = fricition;
    fixture_def.userData.pointer = id;
    body->CreateFixture(&fixture_def);
}

void Shape::update() {
    shape->setPosition(convert_b2vector_to_sf_vector(body->GetPosition()));
    shape->setRotation(body->GetAngle() * 180.0f / b2_pi);
}

class Player : public Shape {
public:
    Player(float x, float y, sf::Texture& texture) : Shape(x,y,texture) {}
    void create_body(b2World& world, bool dynamic = true) override;
    int number_of_contacts = 0;
};

void Player::create_body(b2World& world, bool dynamic) {
    Shape::create_body(world, dynamic);

    body->SetFixedRotation(true);
    
    b2PolygonShape polygon_shape;
    b2FixtureDef fixture_def;
    polygon_shape.SetAsBox(0.3f, 0.3f, b2Vec2(0, 1), 0);
    fixture_def.shape = &polygon_shape;
    fixture_def.density = 1;
    fixture_def.isSensor = true;
    fixture_def.userData.pointer = PLAYER_FOOT_ID;
    body->CreateFixture(&fixture_def);
}

class MyContactListener : public b2ContactListener {
public:
    MyContactListener(Player& player) : player(player) {}
    void BeginContact(b2Contact* contact) override;
    void EndContact(b2Contact* contact) override;

    Player& player;
};

void MyContactListener::BeginContact(b2Contact* contact) {
    auto data = contact->GetFixtureA()->GetUserData();
    if ((int)data.pointer == PLAYER_FOOT_ID) {
        player.number_of_contacts++;
    }

    data = contact->GetFixtureB()->GetUserData();
    if ((int)data.pointer == PLAYER_FOOT_ID) {
        player.number_of_contacts++;
    }
}

void MyContactListener::EndContact(b2Contact* contact) {
    auto data = contact->GetFixtureA()->GetUserData();
    if ((int)data.pointer == PLAYER_FOOT_ID) {
        player.number_of_contacts--;
    }

    data = contact->GetFixtureB()->GetUserData();
    if ((int)data.pointer == PLAYER_FOOT_ID) {
        player.number_of_contacts--;
    }
}


int main() {
    auto settings = sf::ContextSettings();
    settings.antialiasingLevel = 0;
    sf::RenderWindow window(sf::VideoMode(800, 600), "Box2d fun", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(60);

    sf::Texture tileset_texture;
    if (!tileset_texture.loadFromFile("assets/tileset.png")) {
        printf("Failed to load tileset :(");
    }
    //box2d stuff
    b2Vec2 gravity(0.0f, 9.81f);
    b2World world(gravity);
    world.SetAllowSleeping(true);

    float time_step = 1.0f / 60.0f;
    int32 velocity_iterations = 15;
    int32 position_iterations = 15;

    
    Player player(300, 300, tileset_texture);
    player.fricition = 0.0f;
    player.set_as_rectangle(64, 64, true);
    player.create_body(world, true);
    player.get_body().SetGravityScale(3.0f);

    Shape ground(400, 568, tileset_texture);
    ground.id = GROUND_ID;
    ground.set_as_rectangle(800, 64, false);
    ground.create_body(world, false);

    Shape ground2(400, 430, tileset_texture);
    ground2.id = GROUND_ID;
    ground2.set_as_rectangle(200, 64, false);
    ground2.create_body(world, false);

    float speed = 15.0f;
    float jump_force = 135.0f;
    float desired_velocity = 0.f;
    bool player_has_ground = false;

    MyContactListener ground_listener(player);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyReleased) {
                desired_velocity = 0.0f;
            }
        }
        
        world.SetContactListener(&ground_listener);
        player_has_ground = player.number_of_contacts > 0;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
            desired_velocity = -speed;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            desired_velocity = speed;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && player_has_ground) {
            auto mass = player.get_body().GetMass();
            auto force = convert_sf_vector_to_b2vector({ 0.0f, -jump_force });
            force *= mass;
            player.get_body().ApplyLinearImpulse(force, player.get_body().GetWorldCenter(), true);
        }

        auto current_velocity = player.get_body().GetLinearVelocity();
        auto delta_velocity = desired_velocity - current_velocity.x;
        auto impulse = player.get_body().GetMass() * delta_velocity;
        player.get_body().ApplyLinearImpulse(b2Vec2(impulse, 0.0f), player.get_body().GetWorldCenter(), true);


        window.clear();
        world.Step(time_step, velocity_iterations, position_iterations);
        player.update();
        ground.update();
        window.draw(ground.get_shape());
        window.draw(ground2.get_shape());
        window.draw(player.get_shape());
        window.display();
    }

}