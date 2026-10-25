import bpy
import bmesh
import math
import os

OUTPUT_DIR = r"E:\Projekte\Velocitytower\RawAssets"
os.makedirs(OUTPUT_DIR, exist_ok=True)

def clear_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)

def create_mat(name, base_color=(0.8, 0.8, 0.8, 1.0), metallic=0.0, roughness=0.3, emission=(0.0, 0.0, 0.0, 1.0), emission_strength=0.0):
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs["Base Color"].default_value = base_color
        bsdf.inputs["Metallic"].default_value = metallic
        bsdf.inputs["Roughness"].default_value = roughness
        if "Emission Color" in bsdf.inputs:
            bsdf.inputs["Emission Color"].default_value = emission
        elif "Emission" in bsdf.inputs:
            bsdf.inputs["Emission"].default_value = emission
        if "Emission Strength" in bsdf.inputs:
            bsdf.inputs["Emission Strength"].default_value = emission_strength
    return mat

def finalize_and_export(objs, filename):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    if len(objs) > 1:
        bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = os.path.splitext(filename)[0]
    
    # Apply transforms
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    
    # Smart UV Project for clean UVs in Unreal
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(66.0), island_margin=0.02)
    bpy.ops.object.mode_set(mode='OBJECT')
    
    # Smooth shading with weighted normals
    for poly in obj.data.polygons:
        poly.use_smooth = True
    
    out_path = os.path.join(OUTPUT_DIR, filename)
    bpy.ops.export_scene.fbx(
        filepath=out_path,
        use_selection=True,
        global_scale=0.01,
        apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_NONE',
        axis_forward='-Y',
        axis_up='Z',
        object_types={'MESH'},
        use_mesh_modifiers=True,
        mesh_smooth_type='FACE',
        add_leaf_bones=False
    )
    print(f"Exported: {out_path}")

def add_bevel(obj, width=2.0, segments=2):
    bev = obj.modifiers.new(name="Bevel", type='BEVEL')
    bev.width = width
    bev.segments = segments
    bev.limit_method = 'ANGLE'
    bev.angle_limit = math.radians(30)

# 1. SM_Platform_Normal (100x100x36 base unit so scaling X in UE scales width cleanly)
def build_platform_normal():
    clear_scene()
    mat_top = create_mat("MI_PlatTop", (0.0, 0.96, 0.83, 1.0), 0.1, 0.15, (0.0, 0.96, 0.83, 1.0), 3.0)
    mat_body = create_mat("MI_PlatBody", (0.01, 0.52, 0.78, 1.0), 0.2, 0.1)
    mat_frame = create_mat("MI_PlatFrame", (0.04, 0.08, 0.16, 1.0), 0.85, 0.25)
    
    objs = []
    # Top glowing neon ice deck (Z from -5 to 0, so top surface is at Z=0!)
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, -2.5))
    top = bpy.context.active_object
    top.scale = (100.0, 90.0, 5.0)
    add_bevel(top, 0.8, 2)
    top.data.materials.append(mat_top)
    objs.append(top)
    
    # Main crystal body (Z from -20 to -5)
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, -12.5))
    body = bpy.context.active_object
    body.scale = (96.0, 82.0, 15.0)
    add_bevel(body, 1.5, 2)
    body.data.materials.append(mat_body)
    objs.append(body)
    
    # Tech metal side brackets
    for sx in (-46.0, 46.0):
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(sx, 0, -10.0))
        br = bpy.context.active_object
        br.scale = (8.0, 88.0, 18.0)
        add_bevel(br, 0.8, 2)
        br.data.materials.append(mat_frame)
        objs.append(br)
        
    # Under-hanging jagged 3D icicles (spikes)
    spike_positions = [
        (-36, 0, 26), (-18, 12, 34), (0, -8, 40), (18, 10, 32), (36, -4, 24),
        (-28, -18, 22), (10, 20, 28), (28, -16, 25)
    ]
    for (x, y, h) in spike_positions:
        bpy.ops.mesh.primitive_cone_add(vertices=6, radius1=9.0, radius2=0.5, depth=h, location=(x, y, -20.0 - h*0.45))
        cone = bpy.context.active_object
        cone.rotation_euler = (math.pi, 0, x * 0.1)
        cone.data.materials.append(mat_body)
        objs.append(cone)
        
    finalize_and_export(objs, "SM_Platform_Normal.fbx")

# 2. SM_Platform_Boost (Turbo Spring Launchpad)
def build_platform_boost():
    clear_scene()
    mat_gold = create_mat("MI_BoostGold", (1.0, 0.75, 0.04, 1.0), 0.4, 0.2, (1.0, 0.65, 0.0, 1.0), 5.0)
    mat_dark = create_mat("MI_BoostFrame", (0.06, 0.09, 0.18, 1.0), 0.9, 0.2)
    mat_core = create_mat("MI_BoostCore", (1.0, 0.15, 0.52, 1.0), 0.1, 0.1, (1.0, 0.15, 0.52, 1.0), 8.0)
    
    objs = []
    # Base chassis
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, -10.0))
    base = bpy.context.active_object
    base.scale = (100.0, 92.0, 18.0)
    add_bevel(base, 1.5, 2)
    base.data.materials.append(mat_dark)
    objs.append(base)
    
    # Upper launch plate
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, 1.0))
    plate = bpy.context.active_object
    plate.scale = (92.0, 84.0, 4.0)
    add_bevel(plate, 0.8, 2)
    plate.data.materials.append(mat_gold)
    objs.append(plate)
    
    # Glowing Turbine / Spring Dome on top
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=28.0, depth=10.0, location=(0, 0, 5.0))
    dome = bpy.context.active_object
    add_bevel(dome, 1.5, 2)
    dome.data.materials.append(mat_core)
    objs.append(dome)
    
    # Thruster cones underneath
    for sx in (-28.0, 28.0):
        bpy.ops.mesh.primitive_cone_add(vertices=16, radius1=16.0, radius2=8.0, depth=18.0, location=(sx, 0, -24.0))
        th = bpy.context.active_object
        th.data.materials.append(mat_gold)
        objs.append(th)
        
    finalize_and_export(objs, "SM_Platform_Boost.fbx")

# 3. SM_Platform_Ice (Mirror Glacier Slab)
def build_platform_ice():
    clear_scene()
    mat_mirror = create_mat("MI_MirrorIce", (0.88, 0.96, 1.0, 1.0), 0.05, 0.02, (0.5, 0.85, 1.0, 1.0), 2.5)
    mat_crystal = create_mat("MI_DeepGlacier", (0.15, 0.65, 0.98, 1.0), 0.1, 0.05)
    
    objs = []
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, -3.0))
    top = bpy.context.active_object
    top.scale = (100.0, 94.0, 6.0)
    add_bevel(top, 1.2, 3)
    top.data.materials.append(mat_mirror)
    objs.append(top)
    
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, -14.0))
    under = bpy.context.active_object
    under.scale = (94.0, 84.0, 16.0)
    add_bevel(under, 3.0, 2)
    under.data.materials.append(mat_crystal)
    objs.append(under)
    
    # Sleek diamond prism fins underneath
    for sx in (-32, 0, 32):
        bpy.ops.mesh.primitive_cone_add(vertices=4, radius1=14.0, radius2=1.0, depth=30.0, location=(sx, 0, -34.0))
        prism = bpy.context.active_object
        prism.rotation_euler = (math.pi, 0, math.pi * 0.25)
        prism.data.materials.append(mat_mirror)
        objs.append(prism)
        
    finalize_and_export(objs, "SM_Platform_Ice.fbx")

# 4. SM_Platform_Phase (Quantum Phase Emitter)
def build_platform_phase():
    clear_scene()
    mat_holo = create_mat("MI_PhaseHolo", (0.75, 0.52, 0.99, 1.0), 0.0, 0.1, (0.75, 0.52, 0.99, 1.0), 6.0)
    mat_emitter = create_mat("MI_PhaseEmitter", (0.12, 0.06, 0.24, 1.0), 0.9, 0.2)
    
    objs = []
    # Central Holo Grid Slab
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, -4.0))
    slab = bpy.context.active_object
    slab.scale = (96.0, 80.0, 8.0)
    add_bevel(slab, 1.0, 2)
    slab.data.materials.append(mat_holo)
    objs.append(slab)
    
    # Left & Right Quantum Projector Pods
    for sx in (-46.0, 46.0):
        bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=10.0, depth=92.0, location=(sx, 0, -5.0))
        pod = bpy.context.active_object
        pod.rotation_euler = (math.pi * 0.5, 0, 0)
        pod.data.materials.append(mat_emitter)
        objs.append(pod)
        
    finalize_and_export(objs, "SM_Platform_Phase.fbx")

# 5. SM_Platform_Checkpoint (Ornamental Golden Cyber Bridge)
def build_platform_checkpoint():
    clear_scene()
    mat_gold = create_mat("MI_CheckpointGold", (1.0, 0.78, 0.1, 1.0), 0.8, 0.18, (1.0, 0.7, 0.05, 1.0), 3.5)
    mat_steel = create_mat("MI_CheckpointSteel", (0.05, 0.08, 0.15, 1.0), 0.9, 0.25)
    
    objs = []
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, -3.5))
    top = bpy.context.active_object
    top.scale = (100.0, 100.0, 7.0)
    add_bevel(top, 1.0, 2)
    top.data.materials.append(mat_gold)
    objs.append(top)
    
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, -16.0))
    girder = bpy.context.active_object
    girder.scale = (100.0, 90.0, 20.0)
    add_bevel(girder, 1.5, 2)
    girder.data.materials.append(mat_steel)
    objs.append(girder)
    
    finalize_and_export(objs, "SM_Platform_Checkpoint.fbx")

# 6. SM_TowerWall_Segment (3D Cyberpunk Megastructure Wall Pillar, 80x260x400)
def build_tower_wall():
    clear_scene()
    mat_wall = create_mat("MI_WallDark", (0.03, 0.05, 0.12, 1.0), 0.85, 0.25)
    mat_neon = create_mat("MI_WallNeon", (0.0, 0.96, 0.83, 1.0), 0.1, 0.1, (0.0, 0.96, 0.83, 1.0), 6.0)
    mat_chrome = create_mat("MI_WallChrome", (0.15, 0.22, 0.35, 1.0), 0.95, 0.15)
    
    objs = []
    # Main wall column (height 400: Z from 0 to 400)
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, 200.0))
    col = bpy.context.active_object
    col.scale = (60.0, 220.0, 400.0)
    add_bevel(col, 2.0, 2)
    col.data.materials.append(mat_wall)
    objs.append(col)
    
    # Structural horizontal cyber-ribs every 80 units
    for z in (40.0, 120.0, 200.0, 280.0, 360.0):
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(6.0, 0, z))
        rib = bpy.context.active_object
        rib.scale = (66.0, 240.0, 18.0)
        add_bevel(rib, 1.2, 2)
        rib.data.materials.append(mat_chrome)
        objs.append(rib)
        
    # Glowing Inner Neon Energy Rail (along the inner face X=+32)
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(33.0, -35.0, 200.0))
    rail1 = bpy.context.active_object
    rail1.scale = (6.0, 10.0, 400.0)
    rail1.data.materials.append(mat_neon)
    objs.append(rail1)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(33.0, 35.0, 200.0))
    rail2 = bpy.context.active_object
    rail2.scale = (6.0, 10.0, 400.0)
    rail2.data.materials.append(mat_neon)
    objs.append(rail2)
    
    finalize_and_export(objs, "SM_TowerWall_Segment.fbx")

# 7. SM_Climber_Character (Stylized 3D Cyber-Climber, height ~88 cm, origin at center)
def build_climber_character():
    clear_scene()
    mat_hoodie = create_mat("MI_Climber_Hoodie", (0.0, 0.96, 0.83, 1.0), 0.15, 0.35, (0.0, 0.96, 0.83, 1.0), 0.6)
    mat_pants = create_mat("MI_Climber_Pants", (0.12, 0.16, 0.23, 1.0), 0.2, 0.5)
    mat_shoes = create_mat("MI_Climber_Shoes", (0.97, 0.15, 0.52, 1.0), 0.3, 0.25, (0.97, 0.15, 0.52, 1.0), 1.5)
    mat_visor = create_mat("MI_Climber_Visor", (1.0, 1.0, 1.0, 1.0), 0.9, 0.05, (0.0, 0.96, 0.83, 1.0), 8.0)
    mat_skin = create_mat("MI_Climber_Head", (0.99, 0.90, 0.54, 1.0), 0.05, 0.4)
    
    objs = []
    # Torso / Cyber Hoodie
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, 6.0))
    torso = bpy.context.active_object
    torso.scale = (38.0, 26.0, 36.0)
    add_bevel(torso, 6.0, 3)
    torso.data.materials.append(mat_hoodie)
    objs.append(torso)
    
    # Head Sphere
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24, ring_count=16, radius=16.5, location=(0, 0, 36.0))
    head = bpy.context.active_object
    head.data.materials.append(mat_skin)
    objs.append(head)
    
    # Beanie / Cyber Helmet Cap
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24, ring_count=16, radius=17.5, location=(0, 0, 40.0))
    cap = bpy.context.active_object
    cap.scale = (1.02, 1.02, 0.75)
    cap.data.materials.append(mat_shoes)
    objs.append(cap)
    
    # Glowing Cyber Visor (Faces +X so character facing works seamlessly)
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(12.0, 0, 36.0))
    visor = bpy.context.active_object
    visor.scale = (12.0, 26.0, 9.0)
    add_bevel(visor, 2.5, 2)
    visor.data.materials.append(mat_visor)
    objs.append(visor)
    
    # Arms (Left & Right along Y)
    for sy in (-23.0, 23.0):
        bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=6.5, depth=28.0, location=(0, sy, 6.0))
        arm = bpy.context.active_object
        arm.rotation_euler = (math.radians(12 if sy > 0 else -12), 0, 0)
        add_bevel(arm, 2.0, 2)
        arm.data.materials.append(mat_hoodie)
        objs.append(arm)
        
    # Legs (Left & Right)
    for sy in (-10.0, 10.0):
        bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=7.5, depth=26.0, location=(0, sy, -22.0))
        leg = bpy.context.active_object
        add_bevel(leg, 1.5, 2)
        leg.data.materials.append(mat_pants)
        objs.append(leg)
        
        # Cyber Sneakers (bottom at Z = -44)
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(4.0, sy, -38.0))
        shoe = bpy.context.active_object
        shoe.scale = (24.0, 14.0, 12.0)
        add_bevel(shoe, 3.0, 2)
        shoe.data.materials.append(mat_shoes)
        objs.append(shoe)
        
    finalize_and_export(objs, "SM_Climber_Character.fbx")

# 8. SM_Collectible_Core (Faceted Neon Diamond Crystal)
def build_collectible_core():
    clear_scene()
    mat_core = create_mat("MI_NeonCore", (0.0, 0.96, 0.83, 1.0), 0.1, 0.05, (0.0, 0.96, 0.83, 1.0), 10.0)
    objs = []
    bpy.ops.mesh.primitive_cone_add(vertices=8, radius1=22.0, radius2=0.0, depth=26.0, location=(0, 0, 13.0))
    top = bpy.context.active_object
    top.data.materials.append(mat_core)
    objs.append(top)
    
    bpy.ops.mesh.primitive_cone_add(vertices=8, radius1=22.0, radius2=0.0, depth=26.0, location=(0, 0, -13.0))
    bot = bpy.context.active_object
    bot.rotation_euler = (math.pi, 0, 0)
    bot.data.materials.append(mat_core)
    objs.append(bot)
    
    finalize_and_export(objs, "SM_Collectible_Core.fbx")

# 9. SM_PowerUp_Orb (Futuristic Holo-Ring Power-Up Pod)
def build_powerup_orb():
    clear_scene()
    mat_ring = create_mat("MI_PowerUpRing", (1.0, 0.75, 0.04, 1.0), 0.8, 0.15, (1.0, 0.75, 0.04, 1.0), 6.0)
    mat_inner = create_mat("MI_PowerUpInner", (0.97, 0.15, 0.52, 1.0), 0.2, 0.1, (0.97, 0.15, 0.52, 1.0), 10.0)
    
    objs = []
    bpy.ops.mesh.primitive_torus_add(major_radius=24.0, minor_radius=4.5, location=(0, 0, 0))
    torus = bpy.context.active_object
    torus.rotation_euler = (math.pi * 0.5, 0, 0)
    torus.data.materials.append(mat_ring)
    objs.append(torus)
    
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=13.0, location=(0, 0, 0))
    ico = bpy.context.active_object
    ico.data.materials.append(mat_inner)
    objs.append(ico)
    
    finalize_and_export(objs, "SM_PowerUp_Orb.fbx")

# 10. SM_IceChunk (Shattered 3D Ice Crystal Shard)
def build_ice_chunk():
    clear_scene()
    mat_shard = create_mat("MI_IceShard", (0.0, 0.96, 0.83, 1.0), 0.1, 0.1, (0.0, 0.96, 0.83, 1.0), 2.5)
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, radius=14.0, location=(0, 0, 0))
    shard = bpy.context.active_object
    shard.scale = (1.4, 0.85, 0.65)
    shard.data.materials.append(mat_shard)
    finalize_and_export([shard], "SM_IceChunk.fbx")

if __name__ == "__main__":
    build_platform_normal()
    build_platform_boost()
    build_platform_ice()
    build_platform_phase()
    build_platform_checkpoint()
    build_tower_wall()
    build_climber_character()
    build_collectible_core()
    build_powerup_orb()
    build_ice_chunk()
    print("ALL BLENDER 3D ASSETS GENERATED SUCCESSFULLY!")
