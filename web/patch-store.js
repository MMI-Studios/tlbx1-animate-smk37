// Browser-local compact patches. A successful write means IndexedDB committed,
// not merely that bytes were queued. Device flash is a separate backend.
const DATABASE='tlbx-animate-patches';
const STORE='patches';
function open(){return new Promise((resolve,reject)=>{const request=indexedDB.open(DATABASE,1);request.onupgradeneeded=()=>request.result.createObjectStore(STORE);request.onerror=()=>reject(request.error||Error('Patch storage could not be opened.'));request.onblocked=()=>reject(Error('Patch storage is blocked by another tab.'));request.onsuccess=()=>resolve(request.result);});}
async function transaction(mode,operation){const db=await open();try{return await new Promise((resolve,reject)=>{const tx=db.transaction(STORE,mode);let result;tx.oncomplete=()=>resolve(result);tx.onabort=()=>reject(tx.error||Error('Patch storage transaction was aborted.'));tx.onerror=()=>reject(tx.error||Error('Patch storage transaction failed.'));try{const request=operation(tx.objectStore(STORE));request.onsuccess=()=>{result=request.result;};}catch(error){tx.abort();reject(error);}});}finally{db.close();}}
function slotKey(slot){if(!Number.isInteger(slot)||slot<0||slot>15)throw Error('Invalid patch slot.');return slot;}
export async function savePatch(slot,bytes){if(!(bytes instanceof ArrayBuffer)||!bytes.byteLength)throw Error('Cannot save an empty patch.');await transaction('readwrite',store=>store.put(bytes.slice(0),slotKey(slot)));}
export async function loadPatch(slot){const bytes=await transaction('readonly',store=>store.get(slotKey(slot)));if(!(bytes instanceof ArrayBuffer))throw Error('This patch slot is empty.');return bytes;}
export async function listPatches(){return transaction('readonly',store=>store.getAllKeys());}
