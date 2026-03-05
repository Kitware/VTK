PODMAN ?= podman
PODMAN_BUILD_ARGS ?= --format=docker

REPO ?= docker.io/kitware/vtk
TAG_PREFIX ?= ci
BASE_IMAGE ?= fedora42
DATE ?= $(shell date +%Y%m%d)
ARCHES ?= amd64 arm64

# When pushing official tags from another repository.
MIRROR_USER ?= REPLACE_ME
MIRROR_REPO ?= docker.io/$(MIRROR_USER)/vtk
MIRROR_DATE ?= $(shell date +%Y%m%d)

default: manifest-create

build: $(foreach arch,$(ARCHES),build-$(arch))
build-%:
	$(PODMAN) build --platform linux/$* $(DOCKER_BUILD_ARGS) $(BASE_IMAGE) -t $(REPO):$(TAG_PREFIX)-$(BASE_IMAGE)-$*-$(DATE) --progress=plain

push: $(foreach arch,$(ARCHES),push-$(arch))
push-%: build-%
	$(PODMAN) push $(REPO):$(TAG_PREFIX)-$(BASE_IMAGE)-$*-$(DATE)

manifest-create: push
	-$(PODMAN) manifest rm $(REPO):$(TAG_PREFIX)-$(BASE_IMAGE)-$(DATE)
	$(PODMAN) manifest create $(REPO):$(TAG_PREFIX)-$(BASE_IMAGE)-$(DATE) $(foreach arch,$(ARCHES),$(REPO):$(TAG_PREFIX)-$(BASE_IMAGE)-$(arch)-$(DATE))

manifest-push: manifest-create
	$(PODMAN) manifest push $(REPO):$(TAG_PREFIX)-$(BASE_IMAGE)-$(DATE)

manifest-push-mirror:
	skopeo copy --multi-arch all docker://$(MIRROR_REPO):$(TAG_PREFIX)-$(BASE_IMAGE)-$(MIRROR_DATE) docker://$(REPO):$(TAG_PREFIX)-$(BASE_IMAGE)-$(MIRROR_DATE)
