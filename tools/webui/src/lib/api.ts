import type { AceRequest, AceHealth } from './types.js';
import { FETCH_TIMEOUT_MS } from './config.js';

// POST lm: partial request -> enriched request(s)
export async function lmGenerate(req: AceRequest): Promise<AceRequest[]> {
	const res = await fetch('lm', {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(req)
	});
	if (res.status === 503) throw new Error('Server busy');
	if (!res.ok) {
		const err = await res.json().catch(() => ({ error: res.statusText }));
		throw new Error(err.error || res.statusText);
	}
	return res.json();
}

// POST synth[?wav=1]: request(s) -> audio blob(s)
// Metadata (seed, duration, etc) is already in the request JSON from /lm.
export async function synthGenerate(reqs: AceRequest[], format: string): Promise<Blob[]> {
	const url = format === 'wav' ? 'synth?wav=1' : 'synth';
	const body = reqs.length === 1 ? JSON.stringify(reqs[0]) : JSON.stringify(reqs);
	const res = await fetch(url, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body
	});
	if (res.status === 503) throw new Error('Server busy');
	if (!res.ok) {
		const err = await res.json().catch(() => ({ error: res.statusText }));
		throw new Error(err.error || res.statusText);
	}

	const ct = res.headers.get('Content-Type') || '';

	// single track: raw audio body
	if (!ct.startsWith('multipart/')) {
		return [await res.blob()];
	}

	// batch: multipart/mixed, each part is raw audio
	const match = ct.match(/boundary=([^\s;]+)/);
	if (!match) throw new Error('Missing boundary in multipart response');
	const mime = format === 'wav' ? 'audio/wav' : 'audio/mpeg';
	return parseMultipart(new Uint8Array(await res.arrayBuffer()), match[1], mime);
}

// parse multipart/mixed binary response into Blob[].
// each part has only Content-Type header + raw audio body.
function parseMultipart(buf: Uint8Array, boundary: string, mime: string): Blob[] {
	const enc = new TextEncoder();
	const delim = enc.encode('--' + boundary);
	const results: Blob[] = [];

	// find all boundary positions
	const positions: number[] = [];
	for (let i = 0; i <= buf.length - delim.length; i++) {
		let ok = true;
		for (let j = 0; j < delim.length; j++) {
			if (buf[i + j] !== delim[j]) {
				ok = false;
				break;
			}
		}
		if (ok) positions.push(i);
	}

	for (let p = 0; p < positions.length - 1; p++) {
		const partStart = positions[p] + delim.length + 2;
		const partEnd = positions[p + 1] - 2;
		if (partStart >= partEnd) continue;

		// split headers from body at \r\n\r\n
		let splitAt = -1;
		for (let i = partStart; i < partEnd - 3; i++) {
			if (buf[i] === 13 && buf[i + 1] === 10 && buf[i + 2] === 13 && buf[i + 3] === 10) {
				splitAt = i;
				break;
			}
		}
		if (splitAt < 0) continue;

		const body = buf.slice(splitAt + 4, partEnd);
		results.push(new Blob([body], { type: mime }));
	}

	return results;
}

// GET props: server config, pipeline status, default request (2s timeout)
export async function props(): Promise<AceHealth> {
	const res = await fetch('props', {
		signal: AbortSignal.timeout(FETCH_TIMEOUT_MS)
	});
	if (!res.ok) throw new Error('Server unreachable');
	return res.json();
}
