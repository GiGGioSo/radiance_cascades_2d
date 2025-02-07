# radiance\_cascades\_2d

2D implementation of Radiance Cascades (invented by [Alexander Sannikov](https://www.linkedin.com/in/alexander-sannikov-9964aa188/))

![radiance\_cascades\_rays](images/radiance_rays.png)

![radiance\_cascades\_scene](images/randiance_scene.png)

## To-Do (vaguely ordered by priority and ease of implementation)

- [x] Interval overlap: how much of the next inverval overlaps with the previous one (0 to 1)

- [x] Free allocated stuff

- [ ] Bilinear filtering

- [ ] Save resulting render to file

- [ ] Single texture for all of the cascades (they will need to vertically fill the texture)

- [ ] Move cascades calculations on the gpu
    - [ ] cascades creation
    - [ ] cascades merging

- [ ] Calculate cascades for each frame (moving objects)

- [ ] Setup benchmarking environment
    - [ ] Test cases
    - [ ] Logging
    - [ ] Data visualization

## Investigate

- [ ] Different memory layouts
    - [x] for each cascade, probes are adjecent (default)
    - [ ] for each cascade, every direction ray data is adjecent

- [ ] Build and merge at the same time, starting from last cascade
