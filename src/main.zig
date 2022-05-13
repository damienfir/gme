const std = @import("std");
const zm = @import("zmath");

const sf = struct {
    pub usingnamespace @import("sfml");
    pub usingnamespace sf.system;
    pub usingnamespace sf.graphics;
    pub usingnamespace sf.audio;
    pub usingnamespace sf.window;
};

var gpa = std.heap.GeneralPurposeAllocator(.{}){};
const allocator = gpa.allocator();

const Vec = zm.F32x4;

fn vec2(a: f32, b: f32) Vec {
    return zm.f32x4(a, b, 0, 0);
}

const Wall = struct {
    a: Vec,
    b: Vec,
};

const Room = struct {
    walls: []Wall,
};

const Player = struct {
    pos: Vec,
    sprite: sf.CircleShape,
    speed: f32,
    size: f32,
};

const Controls = struct {
    left: bool,
    right: bool,
    up: bool,
    down: bool,
};

const GameState = struct {
    rooms: []Room,
    player: Player,
    width: u32 = 1024,
    height: u32 = 1024,
    clock: sf.Clock,
    controls: Controls,
};

var game: GameState = undefined;

fn update(dt: f32) void {
    if (game.controls.left) {
        game.player.pos[0] -= game.player.speed * dt;
    }

    if (game.controls.right) {
        game.player.pos[0] += game.player.speed * dt;
    }

    if (game.controls.up) {
        game.player.pos[1] -= game.player.speed * dt;
    }

    if (game.controls.down) {
        game.player.pos[1] += game.player.speed * dt;
    }

    game.player.sprite.setPosition(.{ .x = game.player.pos[0], .y = game.player.pos[1] });
}

fn draw(window: *sf.RenderWindow) !void {
    window.clear(sf.Color.Black);

    for (game.rooms) |room| {
        for (room.walls) |wall| {
            const line = [_]sf.Vertex{
                sf.Vertex{ .position = sf.Vector2f.new(wall.a[0], wall.a[1]) },
                sf.Vertex{ .position = sf.Vector2f.new(wall.b[0], wall.b[1]) },
            };
            window.draw(try sf.VertexArray.createFromSlice(&line, .Lines), null);
        }
    }

    window.draw(game.player.sprite, null);

    window.display();
}

pub fn main() anyerror!void {
    game.width = 1024;
    game.height = 1024;

    var window = try sf.RenderWindow.createDefault(.{ .x = game.width, .y = game.height }, "Gme");

    game.player.pos = vec2(100, 100);
    game.player.size = 10;
    game.player.sprite = try sf.CircleShape.create(game.player.size);
    game.player.sprite.setFillColor(sf.Color.Green);
    game.player.speed = 100;

    game.rooms = &[_]Room{
        .{
            .walls = &[_]Wall{
                .{ .a = vec2(10, 10), .b = vec2(200, 10) },
            },
        },
    };

    game.clock = try sf.Clock.create();

    while (window.isOpen()) {
        while (window.pollEvent()) |event| {
            if (event == .closed) {
                window.close();
            }
        }

        game.controls.left = sf.keyboard.isKeyPressed(.A);
        game.controls.right = sf.keyboard.isKeyPressed(.D);
        game.controls.up = sf.keyboard.isKeyPressed(.W);
        game.controls.down = sf.keyboard.isKeyPressed(.S);

        const dt = game.clock.restart().asSeconds();
        update(dt);
        try draw(&window);
    }
}
