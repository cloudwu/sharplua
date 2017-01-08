local sharplua = {}
local _G = _G
local pcall = pcall
local type = type
local getmetatable = getmetatable
local rawget = rawget
_G.sharplua = sharplua	-- make global

local obj_ref = {}
local obj_id = 0
local function fetch_obj(cache, obj)
	if obj_id > 1000000 then	-- max number of cache
		obj_id = #cache
	end
	obj_id = obj_id + 1
	cache[obj] = obj_id
	cache[obj_id] = obj
	obj_ref[obj_id] = obj	-- strong reference
	return obj_id
end

local obj_cache = setmetatable({}, { __index = fetch_obj, __mode = "kv"})
local sharpobj_mt = {}	-- todo: sharp object

function sharplua.unref()
	if next(obj_ref) then
		obj_ref = {}	-- clear strong reference, temp object in lua would be collect later.
	end
end

function sharplua._proxy(obj)
	if getmetatable(obj) == sharpobj_mt then
		return 'S', obj[1]	-- sharp object
	else
		return 'L', obj_cache[obj]	-- lua object
	end
end

local sharp_set = {}

local function fetch_sharp(cache,id)
	local proxy_obj = setmetatable({id}, sharpobj_mt)
	cache[id] = proxy_obj
	sharp_set[id] = true
	return proxy_obj
end

local sharp_cache = setmetatable({}, { __index = fetch_sharp, __mode = "kv" })

function sharplua._object(type, id)
	if type == 'L' then
		-- lua object
		return rawget(obj_cache,id)
	else
		-- type == 'S'
		return sharp_cache[id]
	end
end

function sharplua._garbage()
	for k in pairs(sharp_set) do
		if not rawget(sharp_cache, k) then
			sharp_set[k] = nil
			return k
		end
	end
end

sharplua.call = require "sharplua.cscall"

return sharplua
