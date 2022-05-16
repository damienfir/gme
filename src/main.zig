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

const Line = struct {
    a: Vec,
    b: Vec,
    va: sf.VertexArray,

    pub fn init(x0: f32, y0: f32, x1: f32, y1: f32) !Line {
        const line = [_]sf.Vertex{
            sf.Vertex{ .position = sf.Vector2f.new(x0, y0) },
            sf.Vertex{ .position = sf.Vector2f.new(x1, y1) },
        };
        return Line{
            .a = vec2(x0, y0),
            .b = vec2(x1, y1),
            .va = try sf.VertexArray.createFromSlice(&line, .Lines),
        };
    }

    pub fn normal() Vec {}
};

const Room = struct {
    walls: []Line,
};

const Player = struct {
    pos: Vec,
    sprite: sf.CircleShape,
    speed: f32,
    radius: f32,
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
    var player = game.player;

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
            window.draw(wall.va, null);
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
    game.player.radius = 10;
    game.player.sprite = try sf.CircleShape.create(game.player.radius);
    game.player.sprite.setFillColor(sf.Color.Green);
    game.player.speed = 100;

    game.rooms = &[_]Room{
        .{
            .walls = &[_]Line{
                try Line.init(10, 10, 200, 10),
                try Line.init(200, 10, 200, 200),
                try Line.init(200, 200, 10, 200),
                try Line.init(10, 200, 10, 10),
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
