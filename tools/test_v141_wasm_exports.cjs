// Validate the ABI through the generated JS glue, not minified WASM names.
// Emscripten -Os can change raw export symbols; ccall and cwrap are stable.
const fs = require('node:fs');
const path = require('node:path');
const [wasmPath, jsPath] = process.argv.slice(2);
if (!wasmPath || !jsPath)
 throw new Error('usage: node test_v141_wasm_exports.cjs <wasm> <js>');
const bytes = fs.readFileSync(wasmPath);
if (bytes.length < 200000)
 throw new Error('unexpectedly small WebAssembly: ' + bytes.length);
new WebAssembly.Module(bytes);
const html = fs.readFileSync(path.join(__dirname,'../web-flasher/emulator/index.html'),'utf8');
if (!html.includes('<script src="./squachsim.js"></script>'))
 throw new Error('Page must load classic Emscripten glue as a script');
if (!html.includes("M.cwrap('sw_step'"))
 throw new Error('Browser page does not call the expected firmware API');
const needed = [
 'sw_setup', 'sw_step', 'sw_width', 'sw_height', 'sw_frame',
 'sw_touch', 'sw_detect', 'sw_det_catalog', 'sw_mesh',
 'sw_mesh_status', 'sw_mesh_catalog', 'sw_state', 'sw_rotation'
];
async function main() {
 const factory = require(path.resolve(jsPath));
 if (typeof factory !== 'function')
  throw new Error('The browser glue did not export a SquachSim factory');
 const M = await factory({wasmBinary:new Uint8Array(bytes),
                          print:()=>{}, printErr:()=>{}});
 for (const name of needed) {
  if (typeof M['_'+name] !== 'function')
   throw new Error('Missing browser-callable API: '+name);
 }
 M.ccall('sw_setup',null,[],[]);
 M.ccall('sw_step',null,['number'],[4]);
 const w=M.ccall('sw_width','number',[],[]);
 const h=M.ccall('sw_height','number',[],[]);
 if (w !== 320 || h !== 240)
  throw new Error('Classic CYD framebuffer must be 320x240, got '+w+'x'+h);
 const ptr=M.ccall('sw_frame','number',[],[]);
 if (!ptr || ptr+4*w*h > M.HEAPU8.length)
  throw new Error('Framebuffer pointer is invalid');
 const profiles=JSON.parse(M.ccall('sw_det_catalog','string',[],[]));
 if (!Array.isArray(profiles) || !profiles.length)
  throw new Error('The real detection profile catalog did not load');
 console.log('PASS: current-source browser module, 13 callable APIs, 320x240 framebuffer, '+profiles.length+' detection profiles');
}
main().catch(error=>{ console.error(error); process.exitCode=1; });
