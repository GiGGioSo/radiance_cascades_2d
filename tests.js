
function upper(y, size, size_up) {
    const base_coord = ((y + 0.5) * size / size_up) - 0.5;
    const bilinear_base = Math.floor(base_coord);
    const ratio = (base_coord - Math.trunc(base_coord)) * Math.sign(base_coord);
    return {
        base_coord,
        bilinear_base,
        ratio
    };
}

(async () => {

    const pix_y = 2;
    const cascade0 = upper(2, 1, 4);
    const cascade1 = upper(2, 1, 8);
    console.log(`cascade0: ${cascade0}`);
    console.log(`cascade1: ${cascade1}`);

})();
