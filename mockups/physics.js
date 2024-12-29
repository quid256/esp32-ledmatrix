const num_cells_w = 3 * 4;
const num_cells_h = 5 * 4;

function constrain(num, min, max) {
    return Math.max(min, Math.min(num, max));
}
function thing(char) {
    const cnv = document.createElement("canvas");
    cnv.width = 3 * 4;
    cnv.height = 5 * 4;
    const ctx = cnv.getContext("2d");

    ctx.fillStyle = "#000";
    ctx.fillRect(0, 0, cnv.width, cnv.height);
    ctx.fillStyle = "#FF0000";
    // ctx.font = font;
    // ctx.font = "bold 110px Times New Roman"; //sans-serif";
    // ctx.font = "bold 110px JetBrains Mono"; //sans-serif";
    ctx.font = "bold 23px JetBrains Mono"; //sans-serif";
    ctx.textAlign = "center";
    ctx.fillText(char, cnv.width / 2, cnv.height);

    const imageData = ctx.getImageData(0, 0, cnv.width, cnv.height);

    let cells = new Float32Array(num_cells_w * num_cells_h);
    let num_cells = new Uint32Array(num_cells_w * num_cells_h);
    for (let y = 0; y < cnv.height; y += 1) {
        for (let x = 0; x < cnv.width; x += 1) {
            const index = 4 * (x + y * cnv.width);
            const red = imageData.data[index];
            const i = Math.floor((x * num_cells_w) / cnv.width);
            const j = Math.floor((y * num_cells_h) / cnv.height);
            cells[i + num_cells_w * j] += red / 255.0;
            num_cells[i + num_cells_w * j] += 1;
        }
    }
    for (let i = 0; i < num_cells_w * num_cells_h; i++) {
        cells[i] /= num_cells[i];
    }
    cnv.style.border = "2px solid red";
    document.body.appendChild(cnv);
    return cells;
}
window.addEventListener("load", async () => {
    const cnv = document.querySelector("canvas");
    const ctx = cnv.getContext("2d");
    ctx.fillStyle = "#000";
    ctx.fillRect(0, 0, cnv.width, cnv.height);

    // const ff = new FontFace(
    //     "test",
    //     "url('https://fonts.googleapis.com/css2?family=JetBrains+Mono:ital,wght@0,100..800;1,100..800&display=swap')",
    // );
    // await ff.load();

    cells = thing("0");
    next_cells = thing("2");

    const scale = 0.5;
    const cell_width = (cnv.width / num_cells_w) * scale;
    const cell_height = (cnv.height / num_cells_h) * scale;
    // const offset_x = ((1 - scale) / 2) * cnv.width;
    // const offset_y = ((1 - scale) / 2) * cnv.height;

    for (let j = 0; j < num_cells_h; j++) {
        for (let i = 0; i < num_cells_w; i++) {
            col = cells[i + num_cells_w * j];

            ctx.fillStyle = `rgb(${Math.floor(col * 255.0)}, 0, 0)`;
            ctx.beginPath();
            ctx.arc(
                (5 + i + 0.5) * cell_width,
                (5 + j + 0.5) * cell_height,
                0.4 * Math.min(cell_width, cell_height),
                0,
                2 * Math.PI,
            );
            ctx.fill();
            // ctx.fillRect(i * cell_width, j * cell_height, cell_width, cell_height);
        }
    }
    const num_cells = num_cells_w * num_cells_h;

    // from/to matching
    let from = Array.from(cells);
    let to = Array.from(next_cells);

    function shuffle(array) {
        let currentIndex = array.length;

        // While there remain elements to shuffle...
        while (currentIndex != 0) {
            // Pick a remaining element...
            let randomIndex = Math.floor(Math.random() * currentIndex);
            currentIndex--;

            // And swap it with the current element.
            [array[currentIndex], array[randomIndex]] = [
                array[randomIndex],
                array[currentIndex],
            ];
        }
    }

    from = from.map((k, i) => [i, k]);
    shuffle(from);
    from.sort((a, b) => a[1] - b[1]);
    to = to.map((k, i) => [i, k]);
    shuffle(to);
    to.sort((a, b) => a[1] - b[1]);
    // console.log(from);

    let l = new Uint32Array(num_cells);
    for (let k = 0; k < num_cells; k++) {
        // console.log(l);
        // console.log(from[k], to[k]);
        l[from[k][0]] = to[k][0];
    }
    console.log(l);

    let x = new Float32Array(num_cells);
    let y = new Float32Array(num_cells);
    let dx = new Float32Array(num_cells);
    let dy = new Float32Array(num_cells);
    let tx = new Float32Array(num_cells);
    let ty = new Float32Array(num_cells);
    let colors = new Float32Array(num_cells * 4);

    const eps = 1e-1;
    const s = 200;

    for (let j = 0; j < num_cells_h; j++) {
        for (let i = 0; i < num_cells_w; i++) {
            let k = i + num_cells_w * j;
            x[k] = i;
            y[k] = j;
            dx[k] = eps * s * (Math.random() - 0.5);
            dy[k] = eps * s * (Math.random() - 0.5);
            const to_k = l[k];
            tx[k] = to_k % num_cells_w;
            ty[k] = (to_k / num_cells_w) | 0;
        }
    }

    let started = false;

    document.querySelector("#press").addEventListener("click", () => {
        let digit_cells_list = [];
        for (let i = 0; i < 10; i++) {
            digit_cells_list.push(thing(`${i}`));
        }
        const blob = new Blob(digit_cells_list, {
            type: "application/octet-stream",
        });
        const link = document.createElement("a");
        link.href = window.URL.createObjectURL(blob);
        link.target = "_blank";
        link.download = "particle_clock_data.bin";
        // document.body.appendChild(link);
        link.click();
        // link.remove();
    });

    window.addEventListener("keydown", () => {
        if (started) {
            return;
        }
        started = true;

        setInterval(() => {
            ctx.fillStyle = "#000";
            ctx.fillRect(0, 0, cnv.width, cnv.height);

            for (let k = 0; k < num_cells * 4; k++) {
                colors[k] = 0;
            }
            for (let ik = 0; ik < num_cells; ik++) {
                let k = from[ik][0];
                let i = k % num_cells_w;
                let j = (k / num_cells_w) | 0;
                // let k = i + num_cells_w * j;
                let dist_x = tx[k] - x[k];
                let dist_y = ty[k] - y[k];
                // let dist_x = -(x[k] - (tx[k] + i) / 2);
                // let dist_y = -(y[k] - (ty[k] + j) / 2);
                // let dist_norm = Math.max((dist_x ** 2 + dist_y ** 2) ** 1.5 / 10, 0.2);
                // let dist_norm = (dist_x ** 2 + dist_y ** 2)
                // dx[k] = dx[k] + (eps * dist_x) / dist_norm;
                // dy[k] = dy[k] + (eps * dist_y) / dist_norm;
                //
                dx[k] = dx[k] * 0.85 + eps * 1.0 * dist_x;
                dy[k] = dy[k] * 0.85 + eps * 1.0 * dist_y;
                x[k] += eps * dx[k];
                y[k] += eps * dy[k];

                let pos_x = x[k] + 5;
                let pos_y = y[k] + 5;
                col = cells[i + num_cells_w * j];

                for (let [px, py] of [
                    [Math.floor(pos_x), Math.floor(pos_y)],
                    [Math.floor(pos_x), Math.floor(pos_y) + 1],
                    [Math.floor(pos_x) + 1, Math.floor(pos_y)],
                    [Math.floor(pos_x) + 1, Math.floor(pos_y) + 1],
                ]) {
                    let scalar =
                        (1 - Math.abs(pos_x - px)) * (1 - Math.abs(pos_y - py));
                    if (
                        px >= 0 &&
                        px < num_cells_w * 2 &&
                        py >= 0 &&
                        py < num_cells_h * 2
                    ) {
                        colors[px + py * num_cells_w * 2] = Math.min(
                            colors[px + py * num_cells_w * 2] + scalar * col,
                            1,
                        );
                    }
                }

                // ctx.fillStyle = `rgb(${Math.floor(col * 255.0)}, 0, 0)`;
                // ctx.beginPath();
                // ctx.arc(
                //   (x[k] + 0.5) * cell_width + offset_x,
                //   (y[k] + 0.5) * cell_height + offset_y,
                //   0.4 * Math.min(cell_width, cell_height),
                //   0,
                //   2 * Math.PI,
                // );
                // ctx.fill();
            }

            for (let k = 0; k < num_cells * 4; k++) {
                let i = k % (num_cells_w * 2);
                let j = (k / (num_cells_w * 2)) | 0;
                let col = colors[k];
                ctx.fillStyle = `rgb(${Math.floor(col * 255.0)}, 0, 0)`;
                ctx.beginPath();
                ctx.arc(
                    (i + 0.5) * cell_width,
                    (j + 0.5) * cell_height,
                    0.4 * Math.min(cell_width, cell_height),
                    0,
                    2 * Math.PI,
                );
                ctx.fill();
            }
        }, 40);
    });
});
