
/*
 * Binary format
 * 9*([# variants = N] N*6*[variant encoding])
 * M*([fsindex] [tsindex] [# steps = S] S*[step])
 *
 *
 * where 
 * - variant encoding is just the bitmask of the shape concatted (with 4 trailing 0s)
 * - fsindex, tsindex are 4 bits to specify number + 4 bits to specify variant 
 * - each step is 0, 4 bits for row selection, 2 bits for column selection, 1 bit for orientation
 */
const fs = require("fs");

const VERBOSE = false;

let states = [
  [
    [0b0000, 0b0000, 0b0000, 0b1110, 0b1010, 0b1010, 0b1010, 0b1110, 0b0000, 0b1000, 0b0000],
    [0b0000, 0b0010, 0b0000, 0b1110, 0b1010, 0b1010, 0b1010, 0b1110, 0b0000, 0b0000, 0b0000],
    [0b0000, 0b0100, 0b0000, 0b1110, 0b1010, 0b1010, 0b1010, 0b1110, 0b0000, 0b0000, 0b0000],
    [0b0000, 0b0000, 0b0000, 0b1110, 0b1010, 0b1010, 0b1010, 0b1110, 0b0000, 0b0000, 0b0010],
  ],
  [
    [0b0100,
      0b0110,
      0b0000,
      0b1100,
      0b0100,
      0b0100,
      0b0100,
      0b1110,
      0b0000,
      0b0100,
      0b0010],
    [0b0100, 0b0010, 0b0000, 0b1100, 0b0100, 0b0100, 0b0100, 0b1110, 0b0000, 0b0110, 0b0010],
    [0b1100, 0b1100, 0b0000, 0b1100, 0b0100, 0b0100, 0b0100, 0b1110, 0b0000, 0b0001, 0b0000],
    [0b0000, 0b1100, 0b0000, 0b1100, 0b0100, 0b0100, 0b0100, 0b1110, 0b0000, 0b0111, 0b0000],
  ],
  [
    [0b0000, 0b1000, 0b0000, 0b1110, 0b0010, 0b1110, 0b1000, 0b1110, 0b0000, 0b0000, 0b0010],
    [0b0100, 0b0000, 0b0000, 0b1110, 0b0010, 0b1110, 0b1000, 0b1110, 0b0000, 0b0010, 0b0000],
  ],
  [
    [0b1000, 0b0000, 0b0000, 0b1110, 0b0010, 0b1110, 0b0010, 0b1110, 0b0000, 0b0000, 0b0100],
    [0b0000, 0b0000, 0b0000, 0b1110, 0b0010, 0b1110, 0b0010, 0b1110, 0b0000, 0b1010, 0b0000],
  ],
  [
    [0b0000, 0b1100, 0b0000, 0b1010, 0b1010, 0b1110, 0b0010, 0b0010, 0b0000, 0b0110, 0b0000],
    [0b0010, 0b1100, 0b0000, 0b1010, 0b1010, 0b1110, 0b0010, 0b0010, 0b1000, 0b0000, 0b0000],
  ],
  [
    [0b0000, 0b1010, 0b0000, 0b1110, 0b1000, 0b1110, 0b0010, 0b1110, 0b0000, 0b0000, 0b0000],
    [0b0000, 0b0001, 0b0000, 0b1110, 0b1000, 0b1110, 0b0010, 0b1110, 0b0000, 0b0100, 0b0000],
  ],
  [
    [
      0b0000,
      0b0000,
      0b0000,
      0b1110,
      0b1000,
      0b1110,
      0b1010,
      0b1110,
      0b0000,
      0b1000,
      0b0000],
    [0b0001, 0b0000, 0b0000, 0b1110, 0b1000, 0b1110, 0b1010, 0b1110, 0b0000, 0b0000, 0b0000],
  ],
  [
    [
      0b0010,
      0b0110,
      0b0000,
      0b1110,
      0b0010,
      0b0100,
      0b1000,
      0b1000,
      0b0000,
      0b1110,
      0b0000
    ],
  ],
  [
    [0b0000, 0b0000, 0b0000, 0b1110, 0b1010, 0b1110, 0b1010, 0b1110, 0b0000, 0b0000, 0b0000],
  ],
  [
    [0b0000, 0b0000, 0b0000, 0b1110, 0b1010, 0b1110, 0b0010, 0b1110, 0b0000, 0b0010, 0b0000],
    [0b0000, 0b0100, 0b0000, 0b1110, 0b1010, 0b1110, 0b0010, 0b1110, 0b0000, 0b0000, 0b0000],
  ],
];
//
// states = [
//   [[
//     0b0000, 0b0000, 0b0000, 0b1110, 0b1010, 0b1010, 0b1010, 0b1110, 0b0000, 0b1000, 0b0000,
//   ]],
//   [
//     [0b0100,
//       0b0110,
//       0b0000,
//       0b1100,
//       0b0100,
//       0b0100,
//       0b0100,
//       0b1110,
//       0b0000,
//       0b0100,
//       0b0010],
//   ]
// ]

function bytesToBase64(int_list) {
  bytes = new Uint8Array(int_list);
  const binString = Array.from(bytes, (byte) =>
    String.fromCodePoint(byte),
  ).join("");
  return btoa(binString);
}


if (true) {
  encoded = [];
  for (let s of states) {
    encoded.push(s.length);
    for (let i = 0; i < s.length; i++) {
      c = s[i];
      encoded.push((c[0] << 4) | c[1]);
      encoded.push((c[2] << 4) | c[3]);
      encoded.push((c[4] << 4) | c[5]);
      encoded.push((c[6] << 4) | c[7]);
      encoded.push((c[8] << 4) | c[9]);
      encoded.push(c[10] << 4);
    }
  }
  fs.writeFileSync('out.bin', new Uint8Array(encoded), { flag: 'w' }, () => { });
}

class PriorityQueue {
  constructor(costmap) {
    this.heap = [];
    this.costmap = costmap;
    this.indexMap = new Map();
  }

  // Helper Methods
  getLeftChildIndex(parentIndex) {
    return 2 * parentIndex + 1;
  }
  getRightChildIndex(parentIndex) {
    return 2 * parentIndex + 2;
  }
  getParentIndex(childIndex) {
    return Math.floor((childIndex - 1) / 2);
  }

  hasLeftChild(index) {
    return this.getLeftChildIndex(index) < this.heap.length;
  }
  hasRightChild(index) {
    return this.getRightChildIndex(index) < this.heap.length;
  }
  hasParent(index) {
    return this.getParentIndex(index) >= 0;
  }
  leftChild(index) {
    return this.heap[this.getLeftChildIndex(index)];
  }
  rightChild(index) {
    return this.heap[this.getRightChildIndex(index)];
  }
  parent(index) {
    return this.heap[this.getParentIndex(index)];
  }

  swap(indexOne, indexTwo) {
    const temp = this.heap[indexOne];
    this.heap[indexOne] = this.heap[indexTwo];
    this.heap[indexTwo] = temp;

    this.indexMap.set(this.heap[indexOne], indexOne);
    this.indexMap.set(this.heap[indexTwo], indexTwo);
  }

  peek() {
    if (this.heap.length === 0) {
      return null;
    }
    return this.heap[0];
  }

  // Removing an element will remove the
  // top element with highest priority then
  // heapifyDown will be called
  remove() {
    if (this.heap.length === 0) {
      return null;
    }
    const item = this.heap[0];
    this.heap[0] = this.heap[this.heap.length - 1];
    this.indexMap.set(this.heap[0], 0);
    this.heap.pop();
    this.heapifyDown();
    return item;
  }

  add(item) {
    this.heap.push(item);
    this.indexMap.set(item, this.heap.length - 1);
    this.heapifyUp();
  }

  heapifyUpFrom(index) {
    while (
      this.hasParent(index) &&
      this.costmap.get(this.parent(index)) >
      this.costmap.get(this.heap[index])
    ) {
      this.swap(this.getParentIndex(index), index);
      index = this.getParentIndex(index);
    }
    return index
  }

  heapifyUp() {
    this.heapifyUpFrom(this.heap.length - 1);
  }

  update(val) {
    let new_index = this.heapifyUpFrom(this.indexMap.get(val));
    this.heapifyDownFrom(new_index);
  }

  heapifyDownFrom(index) {
    while (this.hasLeftChild(index)) {
      let smallerChildIndex = this.getLeftChildIndex(index);
      if (
        this.hasRightChild(index) &&
        this.costmap.get(this.rightChild(index)) <
        this.costmap.get(this.leftChild(index))
      ) {
        smallerChildIndex = this.getRightChildIndex(index);
      }
      if (
        this.costmap.get(this.heap[index]) <
        this.costmap.get(this.heap[smallerChildIndex])
      ) {
        break;
      } else {
        this.swap(index, smallerChildIndex);
      }
      index = smallerChildIndex;
    }
  }

  heapifyDown() {
    this.heapifyDownFrom(0);
  }
}

function hashState(s) {
  let h = 0n;
  for (let i = 0n; i < s.length; i++) {
    h |= BigInt(s[i]) << (i * 4n);
  }
  return h;
}

function state_dist(sa, sb) {
  let tot = 0;
  for (let i = 0; i < sa.length; i++) {
    tot += !!(sa[i] ^ sb[i]);
  }
  return tot
}

cases = [];
for (let _i = 0; _i < states.length; _i++) {
  for (let _si = 0; _si < states[_i].length; _si++) {
    for (let _sn = 0; _sn < states[(_i + 1) % states.length].length; _sn++) {
      cases.push([(_i << 4) | _si, ((_i + 1) % states.length << 4) | _sn]);
      cases.push([(_i << 4) | _si, ((_i + 1) % states.length << 4) | _sn]);
      cases.push([(_i << 4) | _si, ((_i + 1) % states.length << 4) | _sn]);
      cases.push([(_i << 4) | _si, ((_i + 1) % states.length << 4) | _sn]);
      cases.push([(_i << 4) | _si, ((_i + 1) % states.length << 4) | _sn]);
    }
  }
}

const PB_LEN = 100

process.stdout.write("[" + " ".repeat(PB_LEN) + "]\r")

for (let _ic = 0; _ic < cases.length; _ic++) {
  let [fsIndex, tsIndex] = cases[_ic];
  let fromState = states[(fsIndex >> 4) & 0x0f][fsIndex & 0x0f];
  let toState = states[(tsIndex >> 4) & 0x0f][tsIndex & 0x0f];

  function add_tostate(g, i, j, mul) {
    for (let k = 0; k < toState.length; k++) {
      for (let l = 0; l < 4; l++) {
        g[k][l] += mul * (Math.abs(k - i) + Math.abs(j - l)) * 1;
      }
    }
  }

  function heuristic(fromstate) {
    let g = Array.from({ length: toState.length }, () => Array.from({ length: 4 }, () => 0));

    for (let i = 0; i < fromstate.length; i++) {
      if (fromstate[i] & 0b1000) {
        add_tostate(g, i, 0, +1);
      }
      if (fromstate[i] & 0b0100) {
        add_tostate(g, i, 1, +1);
      }
      if (fromstate[i] & 0b0010) {
        add_tostate(g, i, 2, +1);
      }
      if (fromstate[i] & 0b0001) {
        add_tostate(g, i, 3, +1);
      }
    }
    for (let i = 0; i < toState.length; i++) {
      if (toState[i] & 0b1000) {
        add_tostate(g, i, 0, -1);
      }
      if (toState[i] & 0b0100) {
        add_tostate(g, i, 1, -1);
      }
      if (toState[i] & 0b0010) {
        add_tostate(g, i, 2, -1);
      }
      if (toState[i] & 0b0001) {
        add_tostate(g, i, 3, -1);
      }
    }

    let v = g.reduce((a, b) => a + b.reduce((x, y) => x + Math.abs(y), 0), 0);
    return v + 5 * Math.random();
  }

  let m = new Map();
  let mh = new Map();
  let ms = new Map();
  let mp = new Map();

  let h = hashState(fromState);
  let surface = new PriorityQueue(mh);
  m.set(h, fromState);
  ms.set(h, 0);
  mh.set(h, heuristic(fromState));
  surface.add(h);

  // let h = heuristic(fromState, tostate_processed)
  // mh.set(surface[0], h)
  // console.log(heuristic(fromState, tostate_processed))

  function neighbors(state, callback) {
    for (let i = 0; i < state.length; i++) {
      let l = (state[i] >> 1) ^ state[i];
      let res;
      if (l & 0b100) {
        let mid = (i << 3) + (0 << 1);
        res = callback(
          state.map((x, j) => (j == i ? x ^ 0b1100 : x)),
          mid,
        );
        if (res) return;
      }
      if (l & 0b010) {
        let mid = (i << 3) + (1 << 1);
        res = callback(
          state.map((x, j) => (j == i ? x ^ 0b0110 : x)),
          mid,
        );
        if (res) return;
      }
      if (l & 0b001) {
        let mid = (i << 3) + (2 << 1);
        res = callback(
          state.map((x, j) => (j == i ? x ^ 0b0011 : x)),
          mid,
        );
        if (res) return;
      }

      if (i > 0) {
        let l = state[i] ^ state[i - 1];
        for (let s = 0; s < 4; s++) {
          let m = [0b1000, 0b0100, 0b0010, 0b0001][s];
          if (l & m) {
            let mid = ((i - 1) << 3) + (s << 1) + 1;
            res = callback(
              state.map((x, j) => (j == i || j == i - 1) ? x ^ m : x),
              mid,
            );
            if (res) return;
          }
        }
      }
    }
  }
  let didit = false;

  while (surface.heap.length > 0 && !didit) {
    let curHash = surface.remove();
    let curState = m.get(curHash);
    let dist = ms.get(curHash);

    let k = Math.round((_ic + 1) / cases.length * PB_LEN)
    process.stdout.write(fsIndex + "," + tsIndex + "; " + dist + " [" + ".".repeat(k) + " ".repeat(PB_LEN - k) + "]\r")

    // displayState(curState)
    neighbors(curState, (n, mid) => {
      let nh = hashState(n);
      if (m.has(nh)) {
        let old_dist = ms.get(nh);
        if (dist + 1 < old_dist) {
          ms.set(nh, dist + 1);
          mh.set(nh, mh.get(nh) + dist + 1 - old_dist);
          mp.set(nh, [curHash, mid]);
          surface.update(nh);
        }
      } else {
        let h = heuristic(n);
        m.set(nh, n);
        ms.set(nh, dist + 1);
        mh.set(nh, dist + 1 + h);
        mp.set(nh, [curHash, mid]);

        if (nh == hashState(toState)) {
          didit = true;
          let curL = nh;
          let pmid;
          let steps = [];
          let grids = [m.get(curL)]
          while (mp.has(curL)) {
            [curL, pmid] = mp.get(curL);
            steps.unshift(pmid);
            grids.unshift(m.get(curL))
          }
          if (VERBOSE) {
            console.log("gl", grids.length, "sl", steps.length)
            for (let i = 0; i < steps.length; i++) {
              console.log(grids[i].map(row => row.toString(2).padStart(4, "0")).join("\n"))
              console.log(steps[i] >> 3, (steps[i] >> 1) & 3, steps[i] & 1)
              console.log(state_dist(grids[i], grids[i + 1]))
            }
            console.log(grids[grids.length - 1].map(row => row.toString(2).padStart(4, "0")).join("\n"))
            console.log("DONE")
          }
          steps.unshift(steps.length);
          steps.unshift(tsIndex);
          steps.unshift(fsIndex);

          // var b64encoded = bytesToBase64(steps);
          fs.writeFileSync('out.bin', new Uint8Array(steps), { flag: 'a' }, () => { });
          // let c = localStorage.getItem("parts");
          // if (!c) {
          //     c = [];
          // } else {
          //     c = JSON.parse(c);
          // }
          // c.push(b64encoded);
          // localStorage.setItem("parts", JSON.stringify(c));
          // console.log("didit");

          return true;
        }
        surface.add(nh);
      }
    });
  }

  let k = Math.round((_ic + 1) / cases.length * PB_LEN)
  process.stdout.write(fsIndex + "," + tsIndex + " [" + ".".repeat(k) + " ".repeat(PB_LEN - k) + "]\r")
}
