local shapelua = {}
local _G = _G
local pcall = pcall
local type = type
local getmetatable = getmetatable
local rawget = rawget
_G.shapelua = shapelua	-- make global

local obj_id = 0
local function fetch_obj(cache, obj)
	if obj_id > 1000000 then	-- max number of cache
		obj_id = #cache
	end
	obj_id = obj_id + 1
	cache[obj] = obj_id
	cache[obj_id] = obj
	return obj_id
end

local obj_cache = setmetatable({}, { __index = fetch_obj, __mode = "kv"})
local shapeobj_mt = {}	-- todo: shape object

function shapelua._proxy(obj)
	if getmetatable(obj) == shapeobj_mt then
		return 'S', obj[1]	-- shape object
	else
		return 'L', obj_cache[obj]	-- lua object
	end
end

local shape_set = {}

local function fetch_shape(cache,id)
	local proxy_obj = setmetatable({id}, shapeobj_mt)
	cache[id] = proxy_obj
	shape_set[id] = true
	return proxy_obj
end

local shape_cache = setmetatable({}, { __index = fetch_shape, __mode = "kv" })

function shapelua._object(type, id)
	if type == 'L' then
		-- lua object
		return rawget(obj_cache,id)
	else
		-- type == 'S'
		return shape_cache[id]
	end
end

function shapelua._garbage()
	for k in pairs(shape_set) do
		if not rawget(shape_cache, k) then
			shape_set[k] = nil
			return k
		end
	end
end

return shapelua
