extends VehicleBody3D
## Полуремарке с контейнер. Отпред има шворц (KINGPIN), който се закачва за седлото на влекача.
## Когато е откачено, стои на опорните крака.

const KINGPIN := Vector3(0, 1.0, 5.0)    # локално
const LENGTH := 12.0

var cargo := 0
var from_depot := -1
var to_depot := -1
var pay := 0
var delivered := false

var legs_shape: CollisionShape3D
var legs_mesh: Node3D
var label: Label3D
var box_mat: StandardMaterial3D


func _mat(color: Color, metallic := 0.0, rough := 0.7) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_color = color
	m.metallic = metallic
	m.roughness = rough
	return m


func _box(size: Vector3, pos: Vector3, mat: Material, parent: Node = self) -> MeshInstance3D:
	var mi := MeshInstance3D.new()
	var bm := BoxMesh.new()
	bm.size = size
	mi.mesh = bm
	mi.position = pos
	mi.material_override = mat
	parent.add_child(mi)
	return mi


func setup(cargo_index: int, color: Color, from_i: int, to_i: int, pay_amount: int, text: String) -> void:
	cargo = cargo_index
	from_depot = from_i
	to_depot = to_i
	pay = pay_amount
	box_mat.albedo_color = color
	label.text = text


func _init() -> void:
	mass = 3200.0
	can_sleep = false
	center_of_mass_mode = RigidBody3D.CENTER_OF_MASS_MODE_CUSTOM
	center_of_mass = Vector3(0, 0.6, 0)

	var body := CollisionShape3D.new()
	var bs := BoxShape3D.new()
	bs.size = Vector3(2.5, 2.9, LENGTH)
	body.shape = bs
	body.position = Vector3(0, 2.45, 0)
	add_child(body)

	legs_shape = CollisionShape3D.new()
	var ls := BoxShape3D.new()
	ls.size = Vector3(2.2, 1.7, 0.4)
	legs_shape.shape = ls
	legs_shape.position = Vector3(0, 0.15, 2.4)
	add_child(legs_shape)

	var dark := _mat(Color(0.13, 0.13, 0.14), 0.3, 0.8)
	box_mat = _mat(Color(0.6, 0.45, 0.25), 0.3, 0.5)
	_box(Vector3(2.5, 0.3, LENGTH), Vector3(0, 1.15, 0), dark)                 # рама
	var cont := _box(Vector3(2.5, 2.6, LENGTH - 0.2), Vector3(0, 2.6, 0), box_mat)
	cont.name = "Container"
	# Ребра на контейнера
	var rib := _mat(Color(0.2, 0.2, 0.22), 0.5, 0.5)
	for i in 7:
		var z := -LENGTH / 2.0 + 0.6 + i * (LENGTH - 1.2) / 6.0
		_box(Vector3(2.56, 2.62, 0.12), Vector3(0, 2.6, z), rib)
	# Задни светлини и броня
	var red := _mat(Color(0.7, 0.05, 0.05))
	red.emission_enabled = true
	red.emission = Color(0.8, 0.05, 0.02)
	_box(Vector3(0.35, 0.2, 0.05), Vector3(-1.0, 0.95, -LENGTH / 2.0 - 0.03), red)
	_box(Vector3(0.35, 0.2, 0.05), Vector3(1.0, 0.95, -LENGTH / 2.0 - 0.03), red)
	_box(Vector3(2.3, 0.2, 0.2), Vector3(0, 0.55, -LENGTH / 2.0 + 0.1), dark)
	# Опорни крака
	legs_mesh = Node3D.new()
	add_child(legs_mesh)
	_box(Vector3(0.18, 1.7, 0.18), Vector3(-0.9, 0.15, 2.4), dark, legs_mesh)
	_box(Vector3(0.18, 1.7, 0.18), Vector3(0.9, 0.15, 2.4), dark, legs_mesh)
	_box(Vector3(0.5, 0.08, 0.5), Vector3(-0.9, -0.7, 2.4), dark, legs_mesh)
	_box(Vector3(0.5, 0.08, 0.5), Vector3(0.9, -0.7, 2.4), dark, legs_mesh)

	for z in [-3.4, -4.7]:
		for x in [-1.05, 1.05]:
			_add_wheel(Vector3(x, 0.0, z))

	label = Label3D.new()
	label.font_size = 96
	label.pixel_size = 0.02
	label.outline_size = 18
	label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	label.position = Vector3(0, 6.0, 0)
	label.modulate = Color(1, 0.9, 0.5)
	label.visibility_range_begin = 22.0     # отблизо не закрива гледката
	label.visibility_range_end = 400.0
	add_child(label)


func _add_wheel(pos: Vector3) -> void:
	var w := VehicleWheel3D.new()
	w.position = pos
	w.wheel_radius = 0.55
	w.wheel_rest_length = 0.3
	w.suspension_travel = 0.3
	w.suspension_stiffness = 50.0
	w.suspension_max_force = 40000.0
	w.damping_compression = 1.8
	w.damping_relaxation = 2.6
	w.wheel_friction_slip = 3.0
	w.wheel_roll_influence = 0.05
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


func kingpin_global() -> Vector3:
	return global_transform * KINGPIN


func set_attached(on: bool) -> void:
	legs_shape.disabled = on
	legs_mesh.position.y = 0.6 if on else 0.0      # краката се прибират
	label.visible = not on and not delivered


func speed_kmh() -> float:
	return linear_velocity.length() * 3.6
