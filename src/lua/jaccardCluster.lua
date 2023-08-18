local beta = 0.32; -- threshold of the number of 1-bits
local layer_bits = 4096 * 8; -- the number of 1-bits of each layer, layer_bits must be the multiple of 8

local function xorMerge(s1, s2, len)
    local ret = {} -- Assuming each element is 1 byte

    for i = 1, len do
        ret[i] = string.char(bit.bxor(s1:byte(i), s2:byte(i))) -- Using bit library for bitwise xor
    end

    return ret
end

local function bpopcount(x)
    local y = string.byte(x)
    local sum = 0
    for i = 0, 7 do
        if bit.band(y, bit.lshift(1, i)) ~= 0 then
            sum = sum + 1
        end
    end

    return sum
end

local function one_bits_num(bitmap, cur_layer_cnt, cur_layer_bits)
    local cnt = 0
    local skip = layer_bits / 8

    local start = (cur_layer_cnt - 1) * skip

    for i = 1, cur_layer_bits / 8 do
        cnt = cnt + bpopcount(bitmap[start + i])
    end

    return cnt
end

local function sizeEstimate(bitmap, layer_cnt)
    local base_layer = 0

    for i = layer_cnt, 2, -1 do
        if one_bits_num(bitmap, i - 1, layer_bits) > layer_bits * beta then
            base_layer = i
            break
        end
    end

    if base_layer == 0 then
        base_layer = 1
    end

    local sum = 0.0

    for i = base_layer, layer_cnt - 1 do
        sum = sum + math.log(1 - 2.0 * one_bits_num(bitmap, i, layer_bits) / layer_bits) /
                  math.log(1.0 - 1.0 * 2 / layer_bits)
    end

    sum = sum + math.log(1 - 1.0 * one_bits_num(bitmap, layer_cnt, 2 * layer_bits) / layer_bits) /
              math.log(1.0 - 1.0 / layer_bits)

    return math.pow(2.0, base_layer - 1) * sum
end

local function mros_jaccard_calculate(m1, m2)
    if m1.layer_cnt ~= m2.layer_cnt then
        return string.format("%.2f", -1)
    end

    local len = (m1.layer_cnt + 1) * (layer_bits / 8)

    local merge = xorMerge(m1.bitmap, m2.bitmap, len)

    local symmetric_diffrenece = sizeEstimate(merge, m1.layer_cnt)

    local tmp = (m1.size + m2.size - symmetric_diffrenece) / (m1.size + m2.size + symmetric_diffrenece)

    if tmp > 0 then
        return string.format("%4f", tmp)
    else
        return string.format("%f", 0)
    end
end

local value1 = redis.pcall("mros.getbitmap", KEYS[1])
local value2 = redis.pcall("mros.getbitmap", KEYS[2])

if (value1['err'] ~= nil or value2['err'] ~= nil) then
    return string.format("%.2f", -1.0);
else
    local m1 = {
        bitmap = value1[2],
        layer_cnt = value1[6],
        size = value1[4]
    }

    local m2 = {
        bitmap = value2[2],
        layer_cnt = value2[6],
        size = value2[4]
    }

    return mros_jaccard_calculate(m1, m2)
end
