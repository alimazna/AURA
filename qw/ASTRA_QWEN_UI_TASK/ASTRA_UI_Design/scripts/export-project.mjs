// Packages the project source into exports/ASTRA_UI_Design.zip (requires the `zip` CLI).
import fs from "node:fs";
import path from "node:path";
import os from "node:os";
import { execSync } from "node:child_process";

const root = process.cwd();
const name = "ASTRA_UI_Design";
const outDir = path.join(root, "exports");
const outFile = path.join(outDir, `${name}.zip`);
const ignore = new Set([
  "node_modules", ".git", ".github", "dist", "build", ".cache", ".temp", "coverage",
  "logs", "exports", ".output", ".vinxi", ".nitro", ".tanstack", ".DS_Store", ".lovable", ".workspace",
]);
const skipFile = (f) => (f.startsWith(".env") && f !== ".env.example") || f.endsWith("~") || f.endsWith(".swp");

const stage = fs.mkdtempSync(path.join(os.tmpdir(), "astra-export-"));
const dest = path.join(stage, name);

function copy(src, dst) {
  fs.mkdirSync(dst, { recursive: true });
  for (const e of fs.readdirSync(src, { withFileTypes: true })) {
    if (ignore.has(e.name) || skipFile(e.name)) continue;
    const s = path.join(src, e.name), d = path.join(dst, e.name);
    if (e.isDirectory()) copy(s, d);
    else if (e.isFile()) fs.copyFileSync(s, d);
  }
}
copy(root, dest);

fs.mkdirSync(outDir, { recursive: true });
fs.rmSync(outFile, { force: true });
execSync(`zip -rq "${outFile}" "${name}"`, { cwd: stage });
fs.rmSync(stage, { recursive: true, force: true });
console.log(`Created ${outFile} (${fs.statSync(outFile).size} bytes)`);
