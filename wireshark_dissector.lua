gaggico_proto = Proto("gaggico", "Gaggico Protocol")

local HEADER_SIZE = 8
local SERVER_PORT = 7494

function field(name, type)
   return {name = name, type = type}
end

local enums = {
   state = {
      [0] = "Off",
      "Standby",
      "Brew",
      "Steam",
      "Backflush",
      "Descale",
   },
   maintenance_type = {
      [0] = "Stop",
      "Backflush",
      "Descale",
   }
}

local s2c_messages = {
   {
      name = "State Change",
      fields = {
         field("Time", "timestamp"),
         field("Machine Start Time", "timestamp"),
         field("New State", "enum.state"),
      }
   },
   {
      name = "Sensor Status",
      fields = {
         field("Temperature", "float"),
         field("Pressure", "float"),
         field("Weight", "float"),
      }
   },
   {
      name = "Settings",
      fields = {},
   },
   {
      name = "Maintenance Status",
      fields = {
         field("Stage", "int32"),
         field("Cycle", "int32"),
      }
   },
}

local c2s_messages = {
   {
      name = "Power",
      fields = {
         field("On", "bool"),
      }
   },
   {
      name = "Settings update",
      fields = {},
   },
   {
      name = "Get Status",
      fields = {},
   },
   {
      name = "Maintenance",
      fields = {
         field("Type", "enum.maintenance_type"),
      }
   },
}

local data_types = {
   timestamp = function(buffer)
      local buf = buffer(0, 8)
      local timestamp = buf:uint64() / 1000
      return os.date("%H:%M:%S", timestamp:tonumber()), buf
   end,
   uint32 = function(buffer)
      local buf = buffer(0, 4)
      return buf:uint(), buf
   end,
   int32 = function(buffer)
      local buf = buffer(0, 4)
      return buf:int(), buf
   end,
   bool = function(buffer)
      local buf = buffer(0, 1)
      return buf:int() > 0, buf
   end,
   float = function(buffer)
      local buf = buffer(0, 4)
      return buf:float(), buf
   end,
}

function parse_msg(msg_type, buffer, tree)
   offset = 0
   for _, field in ipairs(msg_type.fields) do
      if string.sub(field.type, 1, 5) == "enum." then
         local enum_type = string.sub(field.type, 6)
         local enum = enums[enum_type]
         if enum == nil then goto continue end
         local idx, buf = data_types.int32(buffer(offset))
         offset = offset + buf:len()
         local val = enum[idx]
         if val == nil then val = string.format("Unknown (%d)", idx) end
         tree:add(buf, field.name .. ": " .. val)
      else
         local data_type = data_types[field.type]
         if data_type == nil then goto continue end
         local val, buf = data_type(buffer(offset))
         offset = offset + buf:len()
         tree:add(buf, field.name .. ": " .. tostring(val))
      end
      ::continue::
   end
   if offset < buffer:len() then
      tree:add(buffer(offset, buffer:len() - offset),
               "Remaining message data")
   end
end

dst_port_f = Field.new("tcp.dstport")

function gaggico_proto.dissector(buffer, pinfo, tree)
   local buf_len = buffer:captured_len()
   local remaining_buf_len = buf_len
   local offset = 0
   if buf_len ~= buffer:reported_len() then return 0 end
   
   while remaining_buf_len > 0 do
      if remaining_buf_len < HEADER_SIZE then
         pinfo.desegment_len = DESEGMENT_ONE_MORE_SEGMENT
         pinfo.desegment_offset = buf_len
         return
      end
      local magic_buf = buffer(offset, 4)
      local magic = magic_buf:string()
      local msg_len_buf = buffer(offset+4, 4)
      local msg_len = msg_len_buf:uint()
      
      if magic ~= "gaco" then return 0 end
      
      pinfo.cols.protocol = "GAGGICO"
      local subtree = tree:add(gaggico_proto, buffer(offset, 8 + msg_len), "Gaggico Message")
      
      local header = subtree:add(buffer(offset, 8), "Message Header")
      header:add(magic_buf, "Magic value: " .. magic)
      header:add(msg_len_buf, "Message length: " .. msg_len)

      offset = offset + 8
      remaining_buf_len = remaining_buf_len - 8
      if remaining_buf_len < msg_len then
         pinfo.desegment_len = msg_len - remaining_buf_len
         pinfo.desegment_offset = buf_len
         return
      end

      local msg_id_buf = buffer(offset, 4)
      local msg_id = msg_id_buf:uint()

      local msg_c2s = tostring(dst_port_f()) == tostring(SERVER_PORT)
      local messages = msg_c2s and c2s_messages or s2c_messages
      pinfo.cols.protocol = (msg_c2s and "->" or "<-") .. " GAGGICO"
      local msg_type = messages[msg_id]
      local msg_name = msg_type and msg_type.name or "Unknown"
      subtree:add(msg_id_buf, string.format("Message type: %s (%d)", msg_name, msg_id))
      local msg_buf = buffer(offset+4, msg_len - 4)
      if msg_type ~= nil then
         parse_msg(msg_type, msg_buf, subtree)
      else
         subtree:add(msg_buf, "Remaining message data")
      end
      
      remaining_buf_len = remaining_buf_len - msg_len
      offset = offset + msg_len
   end
end
tcp_table = DissectorTable.get("tcp.port")
tcp_table:add(7494, gaggico_proto)
