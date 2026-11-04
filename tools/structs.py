#!/usr/bin/env python3
"""Hotel Mario structure layouts -> one-line C declarations for the decompiler.

Edit the field lists below (offset, type, name, note), then run
    python3 tools/structs.py
which rewrites the structure part of tools/hm_decomp_types.h and writes
src/hm_structs.h (a readable, commented copy for the port).
Notes starting with '?' mark names that are a best guess so far.
"""
import os, re

HERE = os.path.dirname(os.path.abspath(__file__))
SIZES = {'int1': 1, 'uint1': 1, 'int2': 2, 'uint2': 2, 'int4': 4, 'uint4': 4}

# ---------------------------------------------------------------- sub-structures
PLAYER = ('player_t', 0x1c, [            # one per player: [0] Mario, [1] Luigi (gds + 0x7a2)
    (0x00, 'int4', 'score', 'closing a flagged door adds 50'),
    (0x04, 'int2', 'hotel', 'current hotel (open_level loads L<n>_*.rtf)'),
    (0x06, 'int2', 'stage', 'stage within the hotel (0x0e = last)'),
    (0x08, 'int2', 'lives', '1-up item adds one, max 99'),
    (0x0a, 'int2', 'coins', 'coin item adds one'),
    (0x0c, 'int2', 'unk_0c', ''),
    (0x0e, 'int2', 'power', '0 small, 1 big (mushroom), 2 fire flower'),
    (0x10, 'int2', 'unk_10', ''),
    (0x12, 'int1', 'got_bonus', '? set to 1/0 around stage end'),
    (0x13, 'int1', 'unk_13', ''),
    (0x14, 'int1', 'unk_14', '? read by the save code'),
    (0x15, 'int1', 'unk_15', ''),
    (0x16, 'int2', 'progress', '? raised, never lowered; also picks the cut-scene clip (cd_play n+7)'),
    (0x18, 'int2', 'unk_18', 'clamped to >= 0 before a cut-scene'),
    (0x1a, 'int1', 'unk_1a', ''),
    (0x1b, 'uint1', 'flags', 'bits 2 and 3 tested at the start of a game'),
])

OBJ = ('obj_t', 0x4c, [                  # small objects owned by the game: kill stars, fireballs
    (0x00, 'code*', 'handler', 'called handler(obj, gds) every frame'),
    (0x04, 'int1', 'unk_04[0x18]', ''),
    (0x1c, 'int2', 'unk_1c', 'cleared before the init call'),
    (0x1e, 'int2', 'unk_1e', ''),
    (0x20, 'int2', 'active', '? cleared before the init call'),
    (0x22, 'int1', 'unk_22[0x2a]', ''),
])

MARIO = ('mario_t', 0x168, [         # gds->mario points here (0x168 bytes, in video RAM bank B)
    (0x000, 'code*', 'handler', 'reads the pointing device: handler(path, &buttons, &x, &y)'),
    (0x004, 'code*', 'slot[57]', 'compiled sprite per picture slot: slot[n](screen + x + y*384); [24] standing'),
    (0x0e8, 'int1', 'unk_0e8[0x1c]', ''),
    (0x104, 'uint2*', 'door', 'stage_info door/lift cell Mario is at (is_door_elev)'),
    (0x108, 'int2', 'step', 'frame counter within a state (jump: index into jump_height)'),
    (0x10a, 'int2', 'fcount', 'counted up every field'),
    (0x10c, 'int2', 'door_col', 'column found by is_door_elev'),
    (0x10e, 'int2', 'x', 'screen x'),
    (0x110, 'int2', 'y', 'screen y (floor_y - 30 when standing)'),
    (0x112, 'int2', 'floor', 'floor 0 (top) .. 4'),
    (0x114, 'int2', 'old_x', 'where he was drawn last (erase position; lift: where he came from)'),
    (0x116, 'int2', 'old_y', ''),
    (0x118, 'int2', 'old_floor', ''),
    (0x11a, 'int2', 'state_group', 'state_ops row: 0 walk, 1 lift up, 2 lift down, 3 jump, 4 door, 5 win, 6 dying, 7 dead'),
    (0x11c, 'int2', 'saved_group', 'group before dying / winning'),
    (0x11e, 'int2', 'state', 'state_ops column; walk states: bit 0 = facing left'),
    (0x120, 'int2', 'sub', 'phase of a door/lift animation'),
    (0x122, 'int2', 'prev_state', 'walk state before a jump (bit 0 = facing left)'),
    (0x124, 'int2', 'anim', 'picture within the walk cycle'),
    (0x126, 'int2', 'anim_speed', 'fields per walk picture (5; 3 in hotel 3)'),
    (0x128, 'int2', 'anim_count', 'counts down to the next picture; also the hang time at the top of a jump'),
    (0x12a, 'int2', 'lift_used', 'set after riding a lift: leave without pressing again'),
    (0x12c, 'int2', 'lift_moved', 'lift ride: Mario already moved to the other end'),
    (0x12e, 'int2', 'jump_move', 'jump: 1 = moving sideways'),
    (0x130, 'int2', 'unk_130', ''),
    (0x132, 'int2', 'unk_132', ''),
    (0x134, 'int2', 'jump_held', 'button 1 still held since the jump began (higher jump)'),
    (0x136, 'int2', 'jump_hang', 'hanging at the top of a high jump'),
    (0x138, 'int2', 'jump_skip', 'short jump: skip a step on the way down'),
    (0x13a, 'int2', 'jump_drift', 'jump started from standing: drift every other field'),
    (0x13c, 'int2', 'drift_toggle', ''),
    (0x13e, 'int2', 'star_timer', 'star item: 1000 or 7200'),
    (0x140, 'int2', 'invincible', 'star item sets 1'),
    (0x142, 'int1', 'unk_142[4]', ''),
    (0x146, 'int2', 'unk_146', ''),
    (0x148, 'int2', 'fire', 'both buttons: throw a fireball (fire power)'),
    (0x14a, 'int2', 'fire_delay', 'fields before the next fireball'),
    (0x14c, 'int2', 'throw_timer', 'throwing picture shown while > 0'),
    (0x14e, 'int2', 'hurt_timer', 'safe from enemies while > 0 (100 after a hit); flashes while > 30'),
    (0x150, 'int2', 'item_flash', 'items set 0x24: flashes'),
    (0x152, 'int2', 'cooldown[5]', 'counted down each field in mario_coll_det: recent coin, mushroom, flower, 1-up, star'),
    (0x15c, 'int2', 'both_buttons', ''),
    (0x15e, 'int2', 'down_count', 'fields the stick has been held down (duck after 2)'),
    (0x160, 'int2', 'ready', 'the_game waits for this before starting'),
    (0x162, 'int2', 'bumped', 'walked into the screen edge (sound once)'),
    (0x164, 'int2', 'fell', 'set when dying (exit_check ends the stage)'),
    (0x166, 'int2', 'hit', 'set to 1 when Mario stomps or kills an enemy'),
])

ACTOR = ('actor_t', 0x274, [           # room objects (enemies, coins...) driven by the stage module
    (0x000, 'code*', 'handler', 'handler(actor, gds); code lives in the L<n>_s<nn>_sub.o module'),
    (0x004, 'int2', 'type', 'object type (>9 = enemy)'),
    (0x006, 'int2', 'floor', 'home floor (door it lives behind)'),
    (0x008, 'int2', 'col', 'home door column'),
    (0x00a, 'int2', 'x', ''),
    (0x00c, 'int2', 'y', 'floor_y[floor] - 24 when on a floor'),
    (0x00e, 'int2', 'cur_floor', ''),
    (0x010, 'int2', 'drawn_x1', 'x when last drawn in plane-A buffer 1 (draw_a1)'),
    (0x012, 'int2', 'drawn_x0', 'x when last drawn in plane-A buffer 0 (draw_a0)'),
    (0x014, 'int2', 'drawn_y1', ''),
    (0x016, 'int2', 'drawn_y0', ''),
    (0x018, 'int2', 'unk_018', ''),
    (0x01a, 'int2', 'box_x0', 'body box: x+9 .. x+15, y+4 .. y+23 (goomba)'),
    (0x01c, 'int2', 'box_x1', ''),
    (0x01e, 'int2', 'box_y0', ''),
    (0x020, 'int2', 'box_y1', ''),
    (0x022, 'int2', 'cmd', 'message from the_game: 1, 2 (frame phases), 3, 4 = destroy'),
    (0x024, 'int2', 'dir', '0 / 1 (facing), -1 = not decided yet'),
    (0x026, 'int2', 'state', 'row of the handler table'),
    (0x028, 'int2', 'step', 'step within the state (walk_steps index)'),
    (0x02a, 'int2', 'unk_02a', ''),
    (0x02c, 'int2', 'unk_02c', ''),
    (0x02e, 'int2', 'unk_02e', ''),
    (0x030, 'int2', 'appear_count', 'how many more times it comes out of its door'),
    (0x032, 'int2', 'timer', ''),
    (0x034, 'int2', 'delay', 'default 40'),
    (0x036, 'int2', 'unk_036', ''),
    (0x038, 'int2', 'start_mode', '1: starts outside, 2: starts in its doorway'),
    (0x03a, 'int2', 'frame', 'index into frames[]'),
    (0x03c, 'int2', 'unk_03c', ''),
    (0x03e, 'int2', 'hit_x0', 'hit box: x+5 .. x+19, y+9 .. y+23 (goomba)'),
    (0x040, 'int2', 'hit_x1', ''),
    (0x042, 'int2', 'hit_y0', ''),
    (0x044, 'int2', 'hit_y1', ''),
    (0x046, 'int2', 'speed', '2 (4 in hotel 8)'),
    (0x048, 'int2', 'unk_048', ''),
    (0x04a, 'int2', 'unk_04a', ''),
    (0x04c, 'int2', 'unk_04c', ''),
    (0x04e, 'int2', 'unk_04e', ''),
    (0x050, 'code*', 'frames[64]', 'compiled sprite routines: frames[frame](screen + x + y*384)'),
    (0x150, 'code*', 'handlers[25]', 'handlers[state*5 + cmd](actor, gds)'),
    (0x1b4, 'code*', 'walk_steps[24]', 'state 3 (walking): walk_steps[step]'),
    (0x214, 'code*', 'steps_b[16]', ''),
    (0x254, 'int4', 'x_min', '0x22, or 0x4c if column 0 of its floor is a lift'),
    (0x258, 'int4', 'x_max', '0x146, or 0x11c if column 6 of its floor is a lift'),
    (0x25c, 'int1', 'unk_25c[0x18]', ''),
])

ROOM = ('room_t', 0x3b14, [            # 0x3b14 = 0x34 + 24*0x274 + 8   # gds->room: the stage's object table
    (0x00, 'int2', 'n_actors', ''),
    (0x02, 'int2', 'handler_ids[25]', 'which module entry point each actor uses'),
    (0x34, 'actor_t', 'actors[24]', ''),
])

# ---------------------------------------------------------------- the world state
GDS = ('gds', 0x8ca, [
    (0x000, 'int4', 'video_path', 'CD-i /video device'),
    (0x004, 'int4', 'audio_path', 'CD-i audio device'),
    (0x008, 'int4', 'sound_map', 'sm_info() result for the sound map'),
    (0x00c, 'int2', 'unk_00c', ''),
    (0x00e, 'int2', 'unk_00e', ''),
    (0x010, 'int4', 'afx[17]', 'sound effect n = disc data afx[n]..afx[n+1] (playAFX)'),
    (0x054, 'int2', 'unk_054', ''),
    (0x056, 'int4', 'sound_map_no', 'cd_smap() map number, starts at 5'),
    (0x05a, 'int4', 'fct_a', 'plane A field control table (display program)'),
    (0x05e, 'int4', 'fct_b', 'plane B field control table'),
    (0x062, 'int4', 'lct_a', '? plane A line control table'),
    (0x066, 'int4', 'lct_b', '? plane B line control table'),
    (0x06a, 'int2', 'video_mode', '? 0/1: selects a 0x1e00 (20-line) offset - PAL vs NTSC?'),
    (0x06c, 'int4', 'screen_a1', 'plane A screen buffer (0x1aaac bytes)'),
    (0x070, 'int4', 'screen_a0', 'plane A buffer, also the loading screen'),
    (0x074, 'int4', 'screen_b0', 'plane B buffer (0x1aaac bytes)'),
    (0x078, 'int4', 'unk_078', ''),
    (0x07c, 'int4', 'draw_a1', 'screen_a1 + video_mode offset'),
    (0x080, 'int4', 'draw_a0', 'screen_a0 + video_mode offset'),
    (0x084, 'int4', 'draw_screen', 'where sprites are drawn: + x + y*384'),
    (0x088, 'int4', 'unk_088', ''),
    (0x08c, 'int4', 'clean_bg', 'clean background copy used to erase sprites'),
    (0x090, 'int4', 'sprite_gfx', 'sprite graphics for the stage'),
    (0x094, 'int4', 'unk_094', ''),
    (0x098, 'int4', 'clut_a', 'colour table for plane A'),
    (0x09c, 'int4', 'clut_b', 'colour table for plane B'),
    (0x0a0, 'uint1', 'loading_clut[0x184]', 'colour table for the loading screen'),
    (0x224, 'int4', 'bg', 'wide background image (0x29fc8 bytes)'),
    (0x228, 'int4', 'vram_a', 'video RAM bank A (0x69520 bytes); also holds a code module'),
    (0x22c, 'int4', 'vram_b', 'video RAM bank B (0x5a632 bytes); stage module loaded here'),
    (0x230, 'int4', 'intro_module', 'module area (vram_b + 0x1b800); main() runs it'),
    (0x234, 'int4', 'unk_234', 'vram_b + 0x20000'),
    (0x238, 'int4', 'unk_238', 'demo pointer data (demoHelp_pt_coord)'),
    (0x23c, 'int4', 'font', 'font used by alphablit'),
    (0x240, 'int4', 'unk_240', ''),
    (0x244, 'uint1*', 'stage_info', 'stage header: +4 ?, +8 time limit, +0x12 item table'),
    (0x248, 'room_t*', 'room', 'object table for the stage'),
    (0x24c, 'int4', 'unk_24c', 'used by puzzle_2/3'),
    (0x250, 'mario_t*', 'mario', ''),
    (0x254, 'obj_t', 'kill_stars[5]', 'handler kill_star'),
    (0x3d0, 'int2', 'n_fireballs', 'fireballs in flight'),
    (0x3d2, 'obj_t', 'fireballs[2]', 'handler sm_f_ball'),
    (0x46a, 'int2', 'frame', 'frame counter'),
    (0x46c, 'int2', 'unk_46c', ''),
    (0x46e, 'int2', 'unk_46e', ''),
    (0x470, 'uint4', 'pad_buttons', 'pointer device buttons (pt_coord): bit0 button 1, bit1 button 2'),
    (0x474, 'uint4', 'pad_prev', 'buttons last frame (edge detection)'),
    (0x478, 'int4', 'pad_x', 'pointer x (pt_coord)'),
    (0x47c, 'int4', 'pad_x_last', 'last accepted x'),
    (0x480, 'int4', 'pad_y', 'pointer y (pt_coord)'),
    (0x484, 'int4', 'pad_y_last', 'last accepted y'),
    (0x488, 'int4', 'held_b1', 'frames button 1 held'),
    (0x48c, 'int4', 'held_b2', 'frames button 2 held'),
    (0x490, 'int2', 'unk_490', ''),
    (0x492, 'int2', 'unk_492', ''),
    (0x494, 'int2', 'draw_field', '1/2: which of the two screen buffers is being drawn'),
    (0x496, 'int2', 'unk_496', 'set by mario_coll_det'),
    (0x498, 'int2', 'unk_498', ''),
    (0x49a, 'int2', 'unk_49a', ''),
    (0x49c, 'int2', 'floor_y[5]', '72,108,144,180,216'),
    (0x4a6, 'int2', 'door_x[7]', '44,88,132,176,220,264,308'),
    (0x4b4, 'int1', 'unk_4b4[0x74]', ''),
    (0x528, 'int2', 'doors_count', 'number of doors in the stage'),
    (0x52a, 'int2', 'doors_open', 'doors open now: 0 = stage cleared, doors_count = ALL DOORS OPEN'),
    (0x52c, 'int4', 'time_left', 'from stage_info+8; counts down ("TIME UP")'),
    (0x530, 'int2', 'unk_530', ''),
    (0x532, 'int2', 'unk_532', ''),
    (0x534, 'int2', 'unk_534', ''),
    (0x536, 'int2', 'unk_536', ''),
    (0x538, 'int2', 'unk_538', ''),
    (0x53a, 'int2', 'unk_53a', 'set to -1 at stage start'),
    (0x53c, 'int2', 'item_hit', 'item Mario just touched: 1/5 star, 2 1-up, 3 fire flower, 4 coin, 7 mushroom, 10 clear enemies, 11/12 change enemies'),
    (0x53e, 'int2', 'item_kind', 'room_eegg_init'),
    (0x540, 'int4', 'item_slot', 'room_eegg_init: pointer into stage_info item table'),
    (0x544, 'int4', 'item_slot2', 'room_eegg_init'),
    (0x548, 'int4', 'change_handler_a', 'handler given to enemies by item 11'),
    (0x54c, 'int4', 'change_handler_b', 'handler given to enemies by item 12'),
    (0x550, 'int1', 'unk_550[0x46]', ''),
    (0x596, 'int2', 'cur_player', '0 = Mario, 1 = Luigi (index into player[])'),
    (0x598, 'int2', 'two_players', '? other player still in the game'),
    (0x59a, 'int2', 'unk_59a', ''),
    (0x59c, 'int2', 'unk_59c', 'colour cycling'),
    (0x59e, 'int2', 'unk_59e', 'colour cycling'),
    (0x5a0, 'int2', 'unk_5a0', 'colour cycling'),
    (0x5a2, 'uint1', 'cycle_clut[0x200]', 'colour cycling buffer'),
    (0x7a2, 'player_t', 'player[2]', 'index with cur_player'),
    (0x7da, 'int1', 'puzzle_vars[0x40]', 'work area shared by puzzle_2..puzzle_8 (each uses it differently)'),
    (0x81a, 'int2', 'unk_81a', 'puzzle rooms'),
    (0x81c, 'int4', 'unk_81c', 'puzzle rooms'),
    (0x820, 'int4', 'unk_820', ''),
    (0x824, 'int2', 'unk_824', ''),
    (0x826, 'int4', 'unk_826', 'from stage_info+4'),
    (0x82a, 'int4', 'unk_82a', 'from stage_info+4'),
    (0x82e, 'int2', 'unk_82e', ''),
    (0x830, 'int2', 'unk_830', ''),
    (0x832, 'int2', 'pause_ok', '? checked by the pause signal handler'),
    (0x834, 'int4', 'unk_834', 'init_pause'),
    # services table: function pointers handed to the stage modules
    (0x838, 'code*', 'svc_cd_anim', ''), (0x83c, 'code*', 'svc_cd_clut', ''),
    (0x840, 'code*', 'svc_cd_init', ''), (0x844, 'code*', 'svc_cd_mul_bkg', ''),
    (0x848, 'code*', 'svc_cd_play', ''), (0x84c, 'code*', 'svc_cd_smap', ''),
    (0x850, 'code*', 'svc_cd_smq', ''), (0x854, 'code*', 'svc_cd_wait', ''),
    (0x858, 'code*', 'svc_ds_abort', ''), (0x85c, 'code*', 'svc_ds_close', ''),
    (0x860, 'code*', 'svc_ds_dispatch', ''), (0x864, 'code*', 'svc_ds_dispatch_execute', ''),
    (0x868, 'code*', 'svc_ds_link_map_module', ''), (0x86c, 'code*', 'svc_ds_open', ''),
    (0x870, 'int4', 'svc_unk_870', 'set to 0x890 (alphablit)'),
    (0x874, 'code*', 'svc_check_doors', ''), (0x878, 'code*', 'svc_color_cycle', ''),
    (0x87c, 'code*', 'svc_demoHelp_pt_coord', ''), (0x880, 'code*', 'svc_elev_switch', ''),
    (0x884, 'code*', 'svc_fade', ''), (0x888, 'code*', 'svc_init_doors', ''),
    (0x88c, 'code*', 'svc_init_floor_map', ''), (0x890, 'code*', 'svc_load_all', ''),
    (0x894, 'code*', 'svc_player_clut_change', ''), (0x898, 'code*', 'svc_power_check', ''),
    (0x89c, 'code*', 'svc_power_cycle', ''), (0x8a0, 'code*', 'svc_playAFX', ''),
    (0x8a4, 'code*', 'svc_puzzle', ''), (0x8a8, 'code*', 'svc_scoring', ''),
    (0x8ac, 'code*', 'svc_set_up_clut', ''), (0x8b0, 'code*', 'svc_sfb_coll_det', ''),
    (0x8b4, 'code*', 'svc_show_high_score', ''), (0x8b8, 'code*', 'svc_show_restore', ''),
    (0x8bc, 'code*', 'svc_unfade', ''), (0x8c0, 'int4', 'svc_myrtf', ''),
    (0x8c4, 'int4', 'svc_DsPcb', ''),
    (0x8c8, 'int2', 'unk_8c8', ''),
])

STRUCTS = [PLAYER, OBJ, MARIO, ACTOR, ROOM, GDS]
TYPE_SIZE = dict(SIZES, **{'code*': 4})

def elem_size(t):
    if t.endswith('*'):
        return 4
    if t in TYPE_SIZE:
        return TYPE_SIZE[t]
    return {s[0]: s[1] for s in STRUCTS}[t]

def field_size(t, name):
    m = re.search(r'\[(0x[0-9a-f]+|\d+)\]$', name)
    return elem_size(t) * (int(m.group(1), 0) if m else 1)

def layout(sname, size, fields):
    """Return C field list with explicit padding; check overlaps."""
    out, pos, npad = [], 0, 0
    for off, t, name, note in sorted(fields):
        if off < pos:
            raise SystemExit('%s: field %s at 0x%x overlaps previous (ends 0x%x)' % (sname, name, off, pos))
        if off > pos:
            out.append((pos, 'int1', '_pad%x[%d]' % (pos, off - pos), ''))
            npad += 1
        out.append((off, t, name, note))
        pos = off + field_size(t, name)
    if pos > size:
        raise SystemExit('%s: fields run to 0x%x past size 0x%x' % (sname, pos, size))
    if pos < size:
        out.append((pos, 'int1', '_pad%x[%d]' % (pos, size - pos), ''))
    return out

def ctype(t):
    return 'code *' if t == 'code*' else (t[:-1] + ' *' if t.endswith('*') else t + ' ')

def main():
    one_line, pretty = [], ['/* hm_structs.h - Hotel Mario structures (generated by tools/structs.py) */',
                            '#ifndef HM_STRUCTS_H', '#define HM_STRUCTS_H', '#include "hm_types.h"', '']
    for sname, size, fields in STRUCTS:
        lay = layout(sname, size, fields)
        one_line.append('struct %s { %s };' % (sname, ' '.join('%s%s;' % (ctype(t), n) for _, t, n, _ in lay)))
        pretty.append('typedef struct %s {   /* 0x%x bytes */' % (sname, size))
        for off, t, n, note in lay:
            decl = '    %s%s;' % (ctype(t) if t not in [s[0] for s in STRUCTS] else 'struct %s ' % t, n)
            pretty.append('%-44s /* 0x%03x %s */' % (decl, off, note) if note else '%-44s /* 0x%03x */' % (decl, off))
        pretty.append('} %s;' % sname)
        pretty.append('')
    pretty.append('#endif')
    open(os.path.join(HERE, '..', 'src', 'hm_structs.h'), 'w').write('\n'.join(pretty) + '\n')

    p = os.path.join(HERE, 'hm_decomp_types.h')
    old = open(p).read().splitlines()
    rest = [l for l in old if not l.startswith('struct ')]
    i = next(k for k, l in enumerate(rest) if l.startswith('//@global'))
    new = rest[:i] + one_line + [''] + rest[i:]
    open(p, 'w').write('\n'.join(new) + '\n')
    print('structures:', ', '.join('%s (0x%x)' % (s[0], s[1]) for s in STRUCTS))

if __name__ == '__main__':
    main()
