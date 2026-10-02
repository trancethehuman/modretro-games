"""Generate bounded BANKED atlas data units, without runtime ROM pointers.

Only WRAM output buffers cross the banked call boundary. This module neither
registers a scene nor executes a game. The independent atlas oracle verifies
the emitted C pixels against registered collisions and authored water.
"""
import math

PATTERNS_PER_UNIT = 512
INDICES_PER_UNIT = 4096
UNIT_DATA_LIMIT = 8192


def validate(data):
    """Reject layouts the public UBYTE/UWORD API cannot address safely."""
    width, height = data['tile_width'], data['tile_height']
    patterns, indices = data['patterns_2bpp_hex'], data['tile_pattern_indices']
    if not (type(width) is int and type(height) is int and
            20 <= width <= 255 and 12 <= height <= 255):
        raise ValueError('Banked atlas tile dimensions exceed its native API')
    if (len(indices) != width * height or not 0 < len(patterns) <= 65535 or
            any(type(index) is not int or not 0 <= index < len(patterns) for index in indices)):
        raise ValueError('Banked atlas indices do not address its dictionary')
    if any(not isinstance(pattern, str) or len(pattern) != 32 or
           len(bytes.fromhex(pattern)) != 16 for pattern in patterns):
        raise ValueError('Banked atlas pattern is not a native sixteen-byte tile')
    if (PATTERNS_PER_UNIT * 16 > UNIT_DATA_LIMIT or
            INDICES_PER_UNIT * 2 > UNIT_DATA_LIMIT):
        raise ValueError('Banked atlas unit data exceeds its reserved ROM budget')
    if (type(data['width_pixels']) is not int or type(data['height_pixels']) is not int or
            (data['width_pixels'] + 7) // 8 != width or
            (data['height_pixels'] + 7) // 8 != height):
        raise ValueError('Banked atlas padded bounds disagree with tile dimensions')
    districts = data['districts']
    if (not 4 <= len(districts) <= 32 or
            [district['id'] for district in districts] != list(range(len(districts)))):
        raise ValueError('Banked atlas district IDs are not registered in order')
    for district in districts:
        name = district['name']
        if (not isinstance(name, str) or not 1 <= len(name) <= 18 or
                not all(32 <= ord(c) <= 126 for c in name) or '"' in name or '\\' in name or
                any(type(district[axis]) is not int or district[axis] < 0 for axis in ('x', 'y')) or
                district['x'] + 128 > data['width_pixels'] or
                district['y'] + 122 > data['height_pixels']):
            raise ValueError('Banked atlas district exceeds its bounds or native name buffer')


def units(data):
    validate(data)
    return (math.ceil(len(data['patterns_2bpp_hex']) / PATTERNS_PER_UNIT),
            math.ceil(len(data['tile_pattern_indices']) / INDICES_PER_UNIT))


def files(data):
    pattern_units, row_units = units(data)
    result = {}
    declarations = ['/* Generated atlas private BANKED data API. Outputs are WRAM. */',
                    '#ifndef TD_ATLAS_DATA_H', '#define TD_ATLAS_DATA_H',
                    '#include <gbdk/platform.h>']
    prefix = ['/* Generated original atlas data; see content/atlas.json. */',
              '#pragma bank 255', '#include <string.h>', '#include "td_atlas_data.h"']
    for unit in range(pattern_units):
        patterns = data['patterns_2bpp_hex'][unit * PATTERNS_PER_UNIT:(unit + 1) * PATTERNS_PER_UNIT]
        name = f'td_atlas_pattern_unit_{unit}'
        declarations.append(f'void {name}(UWORD local_id,UBYTE *tile16) BANKED;')
        lines = prefix + [f'static const UBYTE patterns[{len(patterns)}][16]={{']
        lines += ['    {' + ','.join(str(v) for v in bytes.fromhex(pattern)) + '},' for pattern in patterns]
        lines += ['};', f'void {name}(UWORD local_id,UBYTE *tile16) BANKED {{',
                  '    memcpy(tile16,patterns[local_id],16);', '}', '']
        result[f'src/td_atlas_patterns_{unit}.c'] = '\n'.join(lines)
    for unit in range(row_units):
        indices = data['tile_pattern_indices'][unit * INDICES_PER_UNIT:(unit + 1) * INDICES_PER_UNIT]
        name = f'td_atlas_row_unit_{unit}'
        declarations.append(f'void {name}(UWORD local_offset,UBYTE count,UWORD *out) BANKED;')
        lines = prefix + [f'static const UWORD indices[{len(indices)}]={{']
        lines += ['    ' + ','.join(map(str, indices[i:i + 16])) + ',' for i in range(0, len(indices), 16)]
        lines += ['};', f'void {name}(UWORD local_offset,UBYTE count,UWORD *out) BANKED {{',
                  '    UBYTE i;for(i=0;i<count;i++)out[i]=indices[local_offset+i];', '}', '']
        result[f'src/td_atlas_rows_{unit}.c'] = '\n'.join(lines)
    declarations += ['#endif', '']
    result['include/td_atlas_data.h'] = '\n'.join(declarations)
    return result


def source(data):
    districts = data['districts']
    pattern_units, row_units = units(data)
    lines = ['/* Generated atlas API; banked data units keep every ROM array bounded. */',
             '#pragma bank 255', '#include <string.h>', '#include "td_atlas.h"',
             '#include "td_atlas_data.h"', '#include "td_district.h"',
             f'typedef char td_atlas_registered_count_matches[(TD_DISTRICT_COUNT=={len(districts)})?1:-1];',
             'typedef char td_atlas_registered_dimensions_match[(TD_DISTRICT_PIXEL_WIDTH==1024&&TD_DISTRICT_PIXEL_HEIGHT==976)?1:-1];',
             f'static const UWORD td_atlas_origins[{len(districts)}][2]={{']
    lines += ['    {' + f"{d['x']},{d['y']}" + '},' for d in districts]
    lines += ['};', f'static const char td_atlas_names[{len(districts)}][19]={{']
    lines += [f'    "{d["name"]}",' for d in districts]
    lines += ['};',
              'UBYTE td_atlas_bounds(UWORD *width_pixels,UWORD *height_pixels) BANKED {',
              '    if(!width_pixels||!height_pixels)return FALSE;',
              '    *width_pixels=TD_ATLAS_WIDTH_PIXELS;*height_pixels=TD_ATLAS_HEIGHT_PIXELS;return TRUE;', '}',
              'UBYTE td_atlas_position(UBYTE district,UWORD local_u,UWORD local_v,UWORD *x,UWORD *y) BANKED {',
              '    if(!x||!y||district>=TD_DISTRICT_COUNT||local_u>=TD_DISTRICT_PIXEL_WIDTH||local_v>=TD_DISTRICT_PIXEL_HEIGHT)return FALSE;',
              '    *x=td_atlas_origins[district][0]+(local_u>>3);',
              '    *y=td_atlas_origins[district][1]+(local_v>>3);return TRUE;', '}',
              'UBYTE td_atlas_row(UBYTE tile_x,UBYTE tile_y,UBYTE count,UWORD *patterns) BANKED {',
              '    UWORD offset,local,room;UBYTE take,written=0;',
              '    if(!patterns||!count||count>TD_ATLAS_VIEW_WIDTH||tile_x>=TD_ATLAS_TILE_WIDTH||tile_y>=TD_ATLAS_TILE_HEIGHT||count>TD_ATLAS_TILE_WIDTH-tile_x)return FALSE;',
              '    offset=(UWORD)tile_y*TD_ATLAS_TILE_WIDTH+tile_x;',
              '    while(written<count){',
              '        local=offset&4095;room=4096-local;',
              '        take=count-written;if(room<take)take=(UBYTE)room;',
              '        switch(offset>>12){']
    lines += [f'        case {unit}:td_atlas_row_unit_{unit}(local,take,patterns+written);break;' for unit in range(row_units)]
    lines += ['        }', '        offset+=take;written+=take;', '    }', '    return TRUE;', '}',
              'UBYTE td_atlas_pattern(UWORD id,UBYTE *tile16) BANKED {',
              '    if(!tile16||id>=TD_ATLAS_PATTERNS)return FALSE;', '    switch(id>>9){']
    lines += [f'    case {unit}:td_atlas_pattern_unit_{unit}(id&511,tile16);break;' for unit in range(pattern_units)]
    lines += ['    }', '    return TRUE;', '}',
              'UBYTE td_atlas_district(UWORD x,UWORD y,char *name19) BANKED {',
              '    UBYTE district;UWORD left,top;',
              '    if(!name19||x>=TD_ATLAS_WIDTH_PIXELS||y>=TD_ATLAS_HEIGHT_PIXELS)return FALSE;',
              '    for(district=0;district<TD_DISTRICT_COUNT;district++){',
              '        left=td_atlas_origins[district][0];top=td_atlas_origins[district][1];',
              '        if(x>=left&&x-left<TD_DISTRICT_PIXEL_WIDTH/TD_ATLAS_SCALE&&y>=top&&y-top<TD_DISTRICT_PIXEL_HEIGHT/TD_ATLAS_SCALE){',
              '            memcpy(name19,td_atlas_names[district],19);return TRUE;', '        }', '    }',
              '    return FALSE;', '}', '']
    return '\n'.join(lines)
