import "regent"

struct DOMConfig {
  quad_file : int8[256],
  output_file : int8[256],
  dump_output : bool,
}

local c = regentlib.c
local cstring = terralib.includec("string.h")

terra DOMConfig:initialize_from_command()
  self.dump_output = false

  var args = c.legion_runtime_get_input_args()
  var i = 0
  while i < args.argc do
    if cstring.strcmp(args.argv[i], "-i") == 0 then
      i = i + 1
      cstring.strncpy(self.quad_file, args.argv[i], 256)
    elseif cstring.strcmp(args.argv[i], "-o") == 0 then
      i = i + 1
      cstring.strncpy(self.output_file, args.argv[i], 256)
      self.dump_output = true
    end
    i = i + 1
  end

  var f = c.fopen(self.quad_file, "r")
  regentlib.assert(f ~= [&c.FILE](nil), "invalid path to a quadrature file")
  c.fclose(f)
  f = c.fopen(self.output_file, "r")
  regentlib.assert(f == [&c.FILE](nil), "the output file already exists")
end

local input_filename = nil
do
  local cnt = 1
  while cnt <= #arg do
    if arg[cnt] == "-i" then
      cnt = cnt + 1
      input_filename = arg[cnt]
    end
    cnt = cnt + 1
  end
  assert(input_filename)
  io.close(assert(io.open(input_filename)))
end

terra DOMConfig.get_number_angles()
  var filename : rawstring = input_filename
  var f = c.fopen(filename, "rb")
  var N   : int64[1]
  c.fscanf(f, "%d\n", N)
  c.fclose(f)
  return N[0]
end

return DOMConfig
