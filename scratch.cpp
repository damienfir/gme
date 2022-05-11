

struct Image {
    int width;
    int height;
    unsigned char* data;
};

struct ImageRead {
    Image image;
    bool valid;
    std::string message;
};

ImageRead read_png(const char* filename) {
    ImageRead result;

    png_image image;
    image.version = PNG_IMAGE_VERSION;
    image.opaque = NULL;

    if (png_image_begin_read_from_file(&image, filename)) {
        image.format = PNG_FORMAT_RGBA;

        result.image.width = image.width;
        result.image.height = image.height;

        png_bytep buffer = (png_bytep)malloc(PNG_IMAGE_SIZE(image));
        if (buffer != NULL) {
            result.image.data = buffer;
            if (png_image_finish_read(&image, NULL, buffer, 0, NULL)) {
                result.valid = true;
                return result;
            } else {
                result.message = image.message;
                free(buffer);
            }
        } else {
            result.message = image.message;
            png_image_free(&image);
        }
    } else {
        result.message = image.message;
    }

    result.valid = false;
    return result;
}

Sprite make_sprite(Imageimage) {
    Sprite s;
    glGenTextures(1, &s.tex);
    glBindTexture(GL_TEXTURE_2D, s.tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        image.data,
    );
    glGenerateMipmap(GL_TEXTURE_2D);

    s.x0 = 0;
    s.y0 = 0;
    s.x1 = 15.f/image.width;
    s.y1 = 15.f/image.height;

    return sprite;
}

Vec2 to_viewport(Vec2 v) {
    float x = 2 * v.x / (float)coord_width - 1;
    float y = - (2 * v.y / (float)coord_height - 1);
    return  Vec2{x,y};
}

void draw_rect(Rect rect) {
    float x0 = 2 * rect.x / (float)coord_width - 1;
    float y0 = - (2 * rect.y / (float)coord_height - 1);
    float x1 = 2 * (rect.x+rect.w)/(float)coord_width - 1;
    float y1 = - (2 * (rect.y+rect.h)/(float)coord_height - 1);

    glBegin(GL_LINE_LOOP);
    glColor3f(1, 1, 1);
    glVertex2f(x0, y0);
    glVertex2f(x1, y0);
    glVertex2f(x1, y1);
    glVertex2f(x0, y1);
    glEnd();
}

void draw_wall(Wall wall) {
    Vec2 a = to_viewport(wall.a);
    Vec2 b = to_viewport(wall.b);

    glBegin(GL_LINES);
    glColor3f(1,1,1);
    glVertex2f(a.x, a.y);
    glVertex2f(b.x, b.y);
    glEnd();
}

void draw_door(Door door) {
    Vec2 a = to_viewport(door.hinge);
    Vec2 b = to_viewport(door.end());

    glBegin(GL_LINES);
    glColor3f(0.5, 0.5, 0.5);
    glVertex2f(a.x, a.y);
    glVertex2f(b.x, b.y);
    glEnd();
}

void draw_player(Player player) {
    Vec2 q = to_viewport(player.pos);
    glPointSize(player.size);
    glBegin(GL_POINTS);
    glColor3f(0,0,1);
    glVertex2f(q.x, q.y);
    glEnd();
}

void draw_player_sprite(Player player) {
    Vec2 q = to_viewport(player.pos);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2d(state.sprite.x0, state.sprite.y0); glVertex2f(q.x-0.1, q.y-0.1);
    glTexCoord2d(state.sprite.x1, state.sprite.y0); glVertex2f(q.x+0.1, q.y-0.1);
    glTexCoord2d(state.sprite.x1, state.sprite.y1); glVertex2f(q.x+0.1, q.y+0.1);
    glTexCoord2d(state.sprite.x0, state.sprite.y1); glVertex2f(q.x-0.1, q.y+0.1);
    glDisable(GL_TEXTURE_2D);
    glEnd();
}
