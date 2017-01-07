local shapelua = require "shapelua"

local csfunc

function init(c)
	csfunc = c
end

function callback(...)
	local ret = shapelua.call(csfunc, ...)
	print("Return:" , ret)
end

