extends VehicleBody3D
## Камионът: шаси, кабина, 6 колела, управление с клавиатура или с бутоните на екрана.
## Напред е +Z на камиона.

const MAX_SPEED_KMH := 95.0
const REVERSE_KMH := 20.0
const ENGINE_POWER := 2600.0     # на всяко задвижващо колело
const BRAKE_POWER := 90.0
const STEER_LOW := 0.55          # максимален волан на място
const STEER_HIGH := 0.14         # максимален волан при висока скорост

var steer_value := 0.0
var cargo_mesh: MeshInstance3D
var brake_light_mat: StandardMaterial3D


func _ready() -> void:
	mass = 2600.0
	center_of_mass_mode = RigidBody3D.CENTER_OF_MASS_MODE_CUSTOM
	center_of_mass = Vector3(0, -0.2, 0.2)
	can_sleep = false

	var shape := CollisionShape3D.new()
	var box := BoxShape3D.new()
	box.size = Vector3(2.4, 1.5, 7.2)
	shape.shape = box
	shape.position = Vector3(0, 1.0, 0)
	add_child(shape)

	_build_body()
	_add_wheel(Vector3(-1.1, 0.0, 2.5), true, false)
	_add_wheel(Vector3(1.1, 0.0, 2.5), true, false)
	_add_wheel(Vector3(-1.1, 0.0, -1.6), false, true)
	_add_wheel(Vector3(1.1, 0.0, -1.6), false, true)
	_add_wheel(Vector3(-1.1, 0.0, -2.9), false, true)
	_add_wheel(Vector3(1.1, 0.0, -2.9), false, true)


func _mat(color: Color, metallic := 0.0, rough := 0.6) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_color = color
	m.metallic = metallic
	m.roughness = rough
	return m


func _box(size: Vector3, pos: Vector3, mat: Material) -> MeshInstance3D:
	var mi := MeshInstance3D.new()
	var bm := BoxMesh.new()
	bm.size = size
	mi.mesh = bm
	mi.position = pos
	mi.material_override = mat
	add_child(mi)
	return mi


func _build_body() -> void:
	var paint := _mat(Color(0.85, 0.18, 0.12), 0.4, 0.35)
	var dark := _mat(Color(0.12, 0.12, 0.13), 0.2, 0.8)
	var chrome := _mat(Color(0.8, 0.82, 0.85), 1.0, 0.2)
	var glass := _mat(Color(0.15, 0.22, 0.3), 0.6, 0.1)

	# Рама
	_box(Vector3(1.6, 0.35, 7.4), Vector3(0, 0.55, -0.1), dark)
	# Кабина
	_box(Vector3(2.45, 2.0, 2.0), Vector3(0, 1.85, 2.45), paint)
	_box(Vector3(2.45, 0.5, 1.4), Vector3(0, 3.05, 2.15), paint)          # спойлер/покрив
	_box(Vector3(2.2, 0.9, 0.05), Vector3(0, 2.25, 3.46), glass)          # предно стъкло
	_box(Vector3(0.05, 0.7, 1.1), Vector3(-1.24, 2.25, 2.6), glass)
	_box(Vector3(0.05, 0.7, 1.1), Vector3(1.24, 2.25, 2.6), glass)
	_box(Vector3(2.3, 0.7, 0.1), Vector3(0, 1.2, 3.47), chrome)           # решетка
	_box(Vector3(2.5, 0.3, 0.3), Vector3(0, 0.7, 3.5), dark)              # броня
	# Фарове
	var light_mat := _mat(Color(1, 0.97, 0.85))
	light_mat.emission_enabled = true
	light_mat.emission = Color(1, 0.95, 0.8)
	_box(Vector3(0.4, 0.2, 0.05), Vector3(-0.85, 0.9, 3.66), light_mat)
	_box(Vector3(0.4, 0.2, 0.05), Vector3(0.85, 0.9, 3.66), light_mat)
	# Платформа отзад
	_box(Vector3(2.4, 0.15, 4.6), Vector3(0, 0.85, -1.3), dark)
	# Стопове
	brake_light_mat = _mat(Color(0.5, 0.05, 0.05))
	brake_light_mat.emission_enabled = true
	brake_light_mat.emission = Color(1, 0.1, 0.05)
	brake_light_mat.emission_energy_multiplier = 0.0
	_box(Vector3(0.4, 0.2, 0.05), Vector3(-1.0, 0.75, -3.62), brake_light_mat)
	_box(Vector3(0.4, 0.2, 0.05), Vector3(1.0, 0.75, -3.62), brake_light_mat)
	# Товар (вижда се, когато има поръчка)
	cargo_mesh = _box(Vector3(2.3, 1.8, 4.3), Vector3(0, 1.85, -1.3), _mat(Color(0.6, 0.45, 0.25)))
	cargo_mesh.visible = false


func _add_wheel(pos: Vector3, steering: bool, traction: bool) -> void:
	var w := VehicleWheel3D.new()
	w.position = pos
	w.use_as_steering = steering
	w.use_as_traction = traction
	w.wheel_radius = 0.55
	w.wheel_rest_length = 0.3
	w.suspension_travel = 0.3
	w.suspension_stiffness = 45.0
	w.suspension_max_force = 30000.0
	w.damping_compression = 1.6
	w.damping_relaxation = 2.4
	w.wheel_friction_slip = 3.2
	w.wheel_roll_influence = 0.08
	add_child(w)

	var mi := MeshInstance3D.new()
	var cyl := CylinderMesh.new()
	cyl.top_radius = 0.55
	cyl.bottom_radius = 0.55
	cyl.height = 0.4
	mi.mesh = cyl
	mi.rotation_degrees = Vector3(0, 0, 90)
	mi.material_override = _mat(Color(0.08, 0.08, 0.08), 0.0, 0.9)
	w.add_child(mi)
	var hub := MeshInstance3D.new()
	var hc := CylinderMesh.new()
	hc.top_radius = 0.28
	hc.bottom_radius = 0.28
	hc.height = 0.42
	hub.mesh = hc
	hub.rotation_degrees = Vector3(0, 0, 90)
	hub.material_override = _mat(Color(0.75, 0.75, 0.78), 1.0, 0.3)
	w.add_child(hub)


## Скорост напред в м/с (отрицателна на заден ход)
func forward_speed() -> float:
	return linear_velocity.dot(global_transform.basis.z)


func speed_kmh() -> float:
	return absf(forward_speed()) * 3.6


func set_cargo(on: bool, color := Color(0.6, 0.45, 0.25)) -> void:
	cargo_mesh.visible = on
	if on:
		(cargo_mesh.material_override as StandardMaterial3D).albedo_color = color


## Изправя камиона на пътя (ако се е обърнал)
func reset_upright() -> void:
	var yaw := global_rotation.y
	global_transform = Transform3D(Basis(Vector3.UP, yaw), global_position + Vector3(0, 1.5, 0))
	linear_velocity = Vector3.ZERO
	angular_velocity = Vector3.ZERO


func _physics_process(delta: float) -> void:
	var gas := Input.get_action_strength("accelerate")
	var brk := Input.get_action_strength("brake")
	var steer_in := Input.get_action_strength("steer_left") - Input.get_action_strength("steer_right")
	var v := forward_speed()
	var kmh := absf(v) * 3.6

	var force := 0.0
	var brake_force := 0.0
	if gas > 0.0:
		if v < -0.5:
			brake_force = BRAKE_POWER * gas          # движи се назад, натиснат газ -> спира
		elif kmh < MAX_SPEED_KMH:
			force = ENGINE_POWER * gas
	if brk > 0.0:
		if v > 0.5:
			brake_force = BRAKE_POWER * brk
		elif kmh < REVERSE_KMH:
			force = -ENGINE_POWER * 0.6 * brk         # заден ход
	if gas == 0.0 and brk == 0.0:
		brake_force = 4.0                             # двигателна спирачка
	if Input.is_action_pressed("handbrake"):
		brake_force = BRAKE_POWER * 1.5
		force = 0.0

	engine_force = force
	brake = brake_force

	var max_steer := lerpf(STEER_LOW, STEER_HIGH, clampf(kmh / 80.0, 0.0, 1.0))
	steer_value = move_toward(steer_value, steer_in * max_steer, delta * 1.8)
	steering = steer_value

	brake_light_mat.emission_energy_multiplier = 2.0 if brake_force > 10.0 else 0.0
