// Executes only the generated portable C tests. No filesystem preopens or env.
const fs = require('node:fs');
const { WASI } = require('node:wasi');
(async () => {
  const wasi = new WASI({ version: 'preview1', args: ['test'], env: {}, preopens: {}, returnOnExit: true });
  const module = await WebAssembly.compile(fs.readFileSync(process.argv[2]));
  const instance = await WebAssembly.instantiate(module, wasi.getImportObject());
  process.exitCode = wasi.start(instance);
})().catch(error => { console.error(error); process.exitCode = 1; });
