local sharplua = require "sharplua"

local csfunc

function init(c)
	csfunc = c
end

function callback(...)
	local ret = sharplua.call(csfunc, ...)
	print("Return:" , ret)
end

