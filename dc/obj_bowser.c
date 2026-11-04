/* Final upper-room boss, L8_s15_subT.o. Native translation of bowser and
 * m_00f88..m_03482; original decompilation is retained under src/modules/bowser.
 * Links use actor indices, never truncated native pointers. */
#include "obj_common.h"
static actor_t *linked(actor_t *a, gds *g, int off) {
    int n = rd25c(a, off);
    return n > 0 && n <= g->room->n_actors ? &g->room->actors[n - 1] : NULL;
}
static void find_link(actor_t *a, gds *g, int off, int type) {
    for (int i = 0; i < g->room->n_actors; i++)
        if (g->room->actors[i].type == type) {
            wr25c(a, off, i + 1);
            return;
        }
}
static void halt_links(actor_t *a, gds *g) {
    actor_t *b = linked(a, g, 4);
    if (b && b->state == 3)
        b->state = 4;
    b = linked(a, g, 12);
    if (b && b->state == 3) {
        b->state = 4;
        b->step = b->unk_02a = 0;
    }
}
static void box(actor_t *a, int kind) {
    a->unk_03c = 1;
    int l = a->dir == 1;
    if (kind == 0) {
        a->box_y0 = a->y + 14;
        a->box_y1 = a->y + 51;
        a->hit_y0 = a->y + 29;
        a->hit_y1 = a->y + 65;
        a->box_x0 = a->x + (l ? 10 : 22);
        a->box_x1 = a->x + (l ? 44 : 56);
        a->hit_x0 = a->x + (l ? 22 : 10);
        a->hit_x1 = a->x + (l ? 56 : 44);
    } else if (kind == 1) {
        a->box_y0 = a->y + 11;
        a->box_y1 = a->y + 52;
        a->hit_y0 = a->y + 25;
        a->hit_y1 = a->y + 65;
        a->box_x0 = a->x + (l ? 11 : 24);
        a->box_x1 = a->x + (l ? 42 : 55);
        a->hit_x0 = a->x + (l ? 25 : 10);
        a->hit_x1 = a->x + (l ? 56 : 41);
    } else {
        a->box_y0 = a->y + 8;
        a->box_y1 = a->y + 48;
        a->hit_y0 = a->y + 19;
        a->hit_y1 = a->y + 63;
        a->box_x0 = a->x + (l ? 12 : 20);
        a->box_x1 = a->x + (l ? 46 : 54);
        a->hit_x0 = a->x + (l ? 32 : 6);
        a->hit_x1 = a->x + (l ? 60 : 34);
    }
}
static void turn_box(actor_t *a) {
    a->unk_03c = 0;
    a->box_x0 = a->x + 16;
    a->box_x1 = a->x + 51;
    a->box_y0 = a->y + 10;
    a->box_y1 = a->y + 65;
}
static void walk_again(actor_t *a) {
    a->step = 0;
    a->unk_02a = 1;
    a->unk_02e = 0;
    a->x_min = 20;
}
static void knock(actor_t *a) {
    a->state = 4;
    a->step = 4;
    a->unk_02a = 34;
    a->unk_02c = 0;
}
static int init(actor_t *a, gds *g) {
    a->state = 1;
    a->x_min = 10;
    a->delay = 20;
    find_link(a, g, 4, 0x3c);
    find_link(a, g, 12, 0x3d);
    for (int i = 0; i < g->room->n_actors; i++)
        if (g->room->actors[i].type == 0x3a) {
            wr25c(a, 8, i);
            break;
        }
    actor_t *b = linked(a, g, 12);
    if (b)
        b->x_min = 1;
    g->unk_530 = 0;
    a->unk_048 = a->unk_04a = -1;
    return 0;
}
static int reset(actor_t *a, gds *g) {
    if (a->timer++ > a->delay) {
        a->timer = 0;
        if (a->delay > 10)
            a->delay -= 5;
        a->state = 1;
        a->unk_04e = 0;
    }
    g->unk_530 = 0;
    return 0;
}
static int arrive(actor_t *a, gds *g) {
    if (g->unk_53a == 0) {
        a->dir = g->mario->x + 14 < 193 ? 1 : 0;
        a->x = a->drawn_x0 = a->drawn_x1 = a->dir ? 304 : 14;
        a->y = a->drawn_y0 = a->drawn_y1 = 2;
        a->cur_floor = 0;
        a->x_max = 0;
        a->hit_x0 = a->hit_x1 = a->hit_y0 = a->hit_y1 = 0;
        a->box_x0 = a->box_x1 = a->box_y0 = a->box_y1 = 0;
        a->unk_02a = 0;
        a->state = 2;
        a->step = 0;
    }
    return 0;
}
static int appear(actor_t *a, gds *g) {
    for (int i = 0; i <= a->unk_02a; i++)
        obj_draw(a, 16 + i, a->x, a->y + 22 * (a->unk_02a - i));
    if (++a->unk_02a > 2) {
        a->unk_02a = 0;
        a->step = 12;
        a->state = 3;
        if (g->unk_53a)
            knock(a);
    }
    return 0;
}
static void walk(actor_t *a, gds *g) {
    mario_t *m = g->mario;
    if (!a->unk_02e) {
        if (a->unk_02a == 4) {
            a->unk_02a--;
            a->unk_02e = 1;
        } else
            a->unk_02a++;
    } else if (!a->unk_02a) {
        a->unk_02a++;
        a->unk_02e = 0;
    } else
        a->unk_02a--;
    if (a->x_max - 1 == m->floor)
        a->x_min--;
    if (a->x_min-- < 2) {
        if ((a->x < 161 || a->dir != 0) && (a->x > 159 || a->dir != 1) && a->x_max != 0) {
            a->step = a->x_max - 1 == m->floor && (RND(g) & 3) ? 14 : 11;
            a->unk_02a = 0;
        } else {
            int choose;
            if (a->x_max - 1 == m->floor)
                choose = !(RND(g) & 1);
            else
                choose = (RND(g) & 3) != 0;
            a->step = choose ? 5 : 14;
            a->unk_02a = 0;
        }
    }
    a->x += a->dir ? -6 : 6;
    a->frame = a->unk_02a + (a->dir ? 6 : 1);
    if (a->x >= 305) {
        a->x = 304;
        a->step = 1;
        a->unk_02a = 0;
    } else if (a->x < 14) {
        a->x = 14;
        a->step = 1;
        a->unk_02a = 0;
    }
    if (abs(a->x - m->x + (a->dir ? -6 : 42)) < 8) {
        if (!world_invulnerable(g) && m->floor == a->x_max && (((m->state_group == 1 || m->state_group == 2) &&
                                      ((m->state == 1 && m->sub == 3) || m->state == 0 ||
                                       (m->state == 2 && (m->sub == 5 || m->sub == 6)))) ||
                                     m->state_group == 4)) {
            m->state_group = 7;
            m->state = 1;
            g->unk_53a = 2;
            a->step = 4;
            a->unk_02a = 0;
            g->unk_530 = 1;
        }
    } else if (!a->step) {
        uint16_t *d = DOOR_CELL(a->x_max, a->x + 32);
        if (d && (*d & 0xf000) == 0x1000 && (RND(g) & 1) == 1) {
            *d = (*d & 0xfff) | 0x2009;
            g->unk_054 = 2;
        }
    }
    a->cur_floor = m->floor;
    box(a, 0);
    if (a->unk_02a == 2)
        wr25c(a, 0, 0x2024);
    unsigned bob = rd25c(a, 0) & 0xffff;
    wr25c(a, 0, rd25c(a, 0) >> 4);
    obj_draw(a, a->dir ? 12 : 11, a->x, a->y - (bob & 15));
}
static void breathe(actor_t *a, gds *g) {
    obj_draw(a, a->dir ? 29 : 28, a->x, a->y);
    int phase = a->unk_02a++;
    if (!phase) {
        g->unk_054 = 13;
        a->frame = a->dir ? 32 : 30;
        box(a, 1);
    } else {
        a->frame = a->dir ? 33 : 31;
        if (a->unk_02a < 41) {
            if (a->unk_02a == 3) {
                actor_t *b = linked(a, g, 4);
                if (b && !b->state) {
                    b->state = 3;
                    b->dir = a->dir;
                    b->x = a->x + (a->dir ? -20 : 52);
                    b->y = a->y - 11;
                }
            } else if (a->unk_02a > 3) {
                int i = rd25c(a, 8);
                for (; i < g->room->n_actors && g->room->actors[i].type == 0x3a; i++)
                    if (!g->room->actors[i].state) {
                        actor_t *b = &g->room->actors[i];
                        b->state = 3;
                        b->dir = a->dir;
                        b->x = a->x + (a->dir ? -20 : 74);
                        b->y = a->y - 11;
                        b->x_min = (RND(g) & 6) + 4;
                        b->unk_036 = -((RND(g) & 6) + 4);
                        break;
                    }
            }
        } else {
            a->frame = a->dir ? 32 : 30;
            walk_again(a);
            actor_t *b = linked(a, g, 4);
            if (b && b->state == 3)
                b->state = 4;
        }
    }
    a->cur_floor = g->mario->floor;
}
static int update(actor_t *a, gds *g) {
    mario_t *m = g->mario;
    int phase;
    switch (a->step) {
    case 0:
        walk(a, g);
        break;
    case 1:
        if (!a->unk_02a)
            turn_box(a);
        a->frame = a->dir ? 15 - a->unk_02a : 13 + a->unk_02a;
        if (++a->unk_02a > 2) {
            a->step = a->unk_02e == -1 ? 6 : 0;
            a->unk_02a = 1;
            a->unk_02e = 0;
            a->dir ^= 1;
        }
        break;
    case 2:
        a->frame = a->dir ? 22 : 20;
        if (!a->unk_02a) {
            halt_links(a, g);
            hm_world->shake = 0;
        }
        if (++a->unk_02a > 4) {
            walk_again(a);
            a->unk_02c = 0;
        }
        break;
    case 3:
        break;
    case 4:
        a->frame = a->unk_02a + (a->dir ? 26 : 24);
        if (++a->unk_02a > 1) {
            g->unk_53a = 0;
            m->saved_group = m->state_group;
            m->state_group = 6;
            m->fell = 1;
            m->state = m->step = 0;
            a->step = 3;
            a->unk_02a = 0;
        }
        break;
    case 5:
        obj_draw(a, a->dir ? 29 : 28, a->x, a->y);
        phase = a->unk_02a++;
        if (!phase) {
            g->unk_054 = 13;
            a->frame = a->dir ? 32 : 30;
            box(a, 1);
        } else {
            a->frame = a->dir ? 33 : 31;
            if (a->unk_02a > 5) {
                a->unk_02a = 0;
                a->x_min = 5;
                a->step = 6;
            }
        }
        a->cur_floor = m->floor;
        break;
    case 6: {
        a->frame = a->dir ? 22 : 20;
        int behind = (m->x < a->x && !a->dir) || (a->x < m->x && a->dir);
        if (a->x_min-- < 1 ||
            (m->floor == a->x_max &&
             (m->state_group == 1 || m->state_group == 2 || m->state_group == 4))) {
            a->unk_02e = 0;
            a->x_min = 20;
            if (behind) {
                a->step = 1;
                a->unk_02a = 0;
            } else {
                a->step = 0;
                a->unk_02a = 1;
            }
        } else if (behind) {
            a->x_min++;
            a->step = 1;
            a->unk_02e = -1;
        } else if (a->x_max == 4 || (m->floor <= a->x_max && a->x_max != 0)) {
            a->step = 7;
            a->x_max--;
        } else {
            a->step = 8;
            a->x_max++;
        }
        a->cur_floor = m->floor;
        box(a, 0);
        break;
    }
    case 7:
    case 8:
        a->frame = a->dir ? 22 : 20;
        if (++a->unk_02a > 1) {
            a->unk_036 = a->step == 7 ? -11 : -2;
            a->step = 9;
            a->unk_02a = 0;
        }
        break;
    case 9:
        a->unk_02a++;
        a->frame = a->dir ? 23 : 21;
        a->x += a->dir ? -6 : 6;
        if (a->x < 14)
            a->x = 14;
        if (a->x > 304)
            a->x = 304;
        a->y += a->unk_036;
        if (a->y >= g->floor_y[a->x_max] - 66 && a->unk_036 > 0) {
            a->y = g->floor_y[a->x_max] - 66;
            a->unk_02a = 0;
            a->step = 10;
        }
        if (a->unk_036 < 16)
            a->unk_036 += 2;
        a->cur_floor = m->floor;
        box(a, 2);
        break;
    case 10:
        if (!a->unk_02a) {
            g->unk_054 = 6;
            box(a, 0);
        }
        a->frame = a->dir ? 22 : 20;
        if (++a->unk_02a < 6)
            hm_world->shake = a->unk_02a & 1 ? 1 : -1;
        else {
            a->unk_02a = 0;
            a->step = 6;
            hm_world->shake = 0;
        }
        break;
    case 11:
        breathe(a, g);
        break;
    case 12:
        a->frame = 19;
        if (!a->unk_02a) {
            a->y = 6;
            turn_box(a);
            g->unk_054 = 6;
        }
        if (++a->unk_02a < 6)
            hm_world->shake = a->unk_02a & 1 ? 1 : -1;
        else {
            a->unk_02a = 0;
            a->step = 13;
            hm_world->shake = 0;
        }
        break;
    case 13:
        if (!a->unk_02a++)
            a->frame = 14;
        else {
            a->frame = a->dir ? 15 : 13;
            walk_again(a);
        }
        break;
    case 14:
        if (!a->unk_02a++) {
            a->frame = a->dir ? 15 : 13;
            turn_box(a);
            a->cur_floor = m->floor;
        } else {
            a->frame = 14;
            a->step = 15;
            a->unk_02a = 0;
        }
        break;
    case 15: {
        if (!a->unk_02a) {
            a->hit_x0 = a->x + 6;
            a->hit_x1 = a->x + 60;
            a->hit_y0 = a->y + 6;
            a->hit_y1 = a->y + 20;
            a->unk_03c = 1;
        }
        a->cur_floor = m->floor;
        actor_t *b = linked(a, g, 12);
        if (a->unk_02a == 4 && b && !b->state) {
            b->state = 3;
            b->y = 2;
            b->x = (RND(g) & 31) * 10 + 32;
            g->unk_054 = 8;
        }
        for (int i = 0; i < 2; i++)
            obj_draw(a, 16 + i, a->x, a->y + 22 * (2 - i));
        a->frame = 18;
        if (++a->unk_02a > 39) {
            a->step = 13;
            a->unk_02a = 0;
            if (b && b->state == 3) {
                b->state = 4;
                b->step = b->unk_02a = 0;
            }
        }
        break;
    }
    }
    draw_at_xy(a, g, a->frame);
    return 0;
}
static int stomp(actor_t *a, gds *g) {
    if (!g->unk_496) {
        if (a->step == 2 || (a->step == 9 && a->unk_02a))
            knock(a);
        else {
            a->step = 2;
            a->unk_02a = 0;
        }
    } else
        a->unk_04e = a->cmd;
    return 0;
}
static int destroy(actor_t *a, gds *g) {
    if (!g->unk_53a)
        return 2;
    knock(a);
    return 0;
}
static int fade(actor_t *a, gds *g) {
    switch (a->step) {
    case 4:
        a->frame = a->unk_02a;
        draw_at_xy(a, g, a->frame);
        a->unk_02a++;
        a->step = 5;
        halt_links(a, g);
        hm_world->shake = 0;
        break;
    case 5:
        obj_draw(a, 34, a->x, a->y);
        a->frame = a->unk_02a;
        draw_at_xy(a, g, a->frame);
        a->unk_02a++;
        a->step = 6;
        break;
    case 6:
        a->y += 16;
        a->frame = a->unk_02a;
        draw_at_xy(a, g, a->frame);
        if (a->y > 175) {
            a->step = 3;
            a->unk_02a = 2;
        }
        break;
    case 3:
        if (a->unk_02a-- < 2) {
            a->state = 0;
            a->cmd = 2;
            a->timer = 0;
            a->dir = -1;
        }
        break;
    }
    return 0;
}
int bowser(actor_t *a, gds *g) {
    if (!a->state && !a->cmd) {
        obj_tables(a);
        H(0, 0, init);
        H(0, 2, reset);
        H(1, 2, arrive);
        H(2, 2, appear);
        H(3, 2, update);
        H(3, 3, stomp);
        H(3, 4, destroy);
        H(4, 2, fade);
    }
    return obj_dispatch(a, g);
}
