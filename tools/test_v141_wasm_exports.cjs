// Binary-level verification of the current-source Emscripten export ABI.
// This checks the compiled WASM, not just a plausible string in the page.
const fs = require('node:fs');
const path = require('node:path');
const [wasmPath, jsPath] = process.argv.slice(2);
if (!wasmPath || !jsPath)
 throw new Error('usage: node test_v141_wasm_exports.cjs <wasm> <js>');
const bytes = fs.readFileSync(wasmPath);
if (bytes.length < 200000) throw new Error('unexpectedly small WebAssembly');
const wasmModule = new WebAssembly.Module(bytes);
const wasmExportNames = new Set(WebAssembly.Module.exports(wasmModule).map(e => e.name));
const needed = [
  'sw_setup', 'sw_step', 'sw_width', 'sw_height', 'sw_frame',
  'sw_touch', 'sw_detect', 'sw_det_catalog', 'sw_mesh',
  'sw_mesh_status', 'sw_mesh_catalog', 'sw_state', 'sw_rotation'
];
for (const name of needed) {
 if (!wasmExportNames.has(name) && !wasmExportNames.has('_'+name))
  throw new Error('Missing current browser ABI '+name);
}
const glue = fs.readFileSync(jsPath,'utf8');
if (!glue.includes('SquachSim'))
 throw new Error('No Emscripten SquachSim factory');
const html = fs.readFileSync(path.join(__dirname,'../web-flasher/emulator/index.html'),'utf8');
if (!html.includes('<script src="./squachsim.js"></script>'))
 throw new Error('The classic Emscripten JS glue must load as script');
if (!html.includes("M.cwrap('sw_step'"))
 throw new Error('The page does not call the real WASM step function');
console.log('PASS: current-source 2.8 CYD WebAssembly ('+bytes.length+
  ' bytes) includes all '+needed.length+' required ABI exports');
