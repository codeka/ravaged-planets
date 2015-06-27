<?xml version="1.0" ?>
<shader version="1">
  <function name="calculate_shadow_factor"><![CDATA[
    #define SMAP_SIZE 1024
    #define SHADOW_EPSILON 0.0005f

    uniform sampler2DShadow shadow_map;

    // this function samples the shadow map texture and determines whether the given
    // point is in shadow or not
    float calculate_shadow_factor(in vec4 light_pos) {
        //transform from RT space to texture space.
        vec2 shadow_uv = (0.5 * light_pos.xy / light_pos.w) + vec2(0.5, 0.5);
        shadow_uv.y = 1.0 - shadow_uv.y;

        // Determine the lerp amounts
        vec2 lerps = fract(shadow_uv * SMAP_SIZE);
        float delta_uv = 1.0/SMAP_SIZE;

        // Read in bilerp stamp, doing the shadow checks
        float light_pos_z = light_pos.z / light_pos.w;
        float light_amount = 1.0;
        if (texture(shadow_map, shadow_uv).z < light_pos_z) {
          light_amount = 0.5;
        }

        float sourcevals[4];
        sourcevals[0] = step(light_pos_z, texture(shadow_map, shadow_uv + vec2(0, 0)) + SHADOW_EPSILON);
        sourcevals[1] = step(light_pos_z, texture(shadow_map, shadow_uv + vec2(delta_uv, 0)) + SHADOW_EPSILON);
        sourcevals[2] = step(light_pos_z, texture(shadow_map, shadow_uv + vec2(0, delta_uv)) + SHADOW_EPSILON);
        sourcevals[3] = step(light_pos_z, texture(shadow_map, shadow_uv + vec2(delta_uv, delta_uv)) + SHADOW_EPSILON);

        // lerp between the shadow values to calculate our light amount
        float light_amount = mix(mix(sourcevals[0], sourcevals[1], lerps.x),
                                 mix(sourcevals[2], sourcevals[3], lerps.x),
                                 lerps.y);

        return light_amount;
    }
  ]]></function>
</shader>