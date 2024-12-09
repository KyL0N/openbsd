/*      $OpenBSD: stfcrypto.c,v 1.0 2024/12/09 12:00:00 RyuGi Exp $     */
/*
 * Copyright (c) 2024 Ryu Gi <eluniverso2333@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/fdt.h>

#include <dev/ofw/fdt.h>
#include <dev/ofw/ofw_clock.h>
#include <dev/ofw/openfirm.h>

#include <crypto/cryptodev.h>
#include <crypto/cryptosoft.h>
#include <crypto/xform.h>

#define HREAD4(sc, reg) (bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg)))
#define HWRITE4(sc, reg, val) \
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))

struct stfcrypto_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	struct mutex sc_mtx;
	void *sc_ih;

	int32_t sc_cid;

	struct dmac_channel *sc_tx;
	struct dmac_channel *sc_rx;
};

int stfcrypto_match(struct device *, void *, void *);
void stfcrypto_attach(struct device *, struct device *, void *);

const struct cfattach stfcrypto_ca = { sizeof(struct stfcrypto_softc),
	stfcrypto_match, stfcrypto_attach };

struct cfdriver stfcrypto_cd = { NULL, "stfcrypto", DV_DULL };

int
stfcrypto_match(struct device *parent, void *match, void *aux)
{
	struct fdt_attach_args *faa = aux;
	return OF_is_compatible(faa->fa_node, "starfive,jh7110-crypto");
}

void
stfcrypto_attach(struct device *parent, struct device *self, void *aux)
{
	int algs[CRYPTO_ALGORITHM_MAX + 1];
	struct stfcrypto_softc *sc = (struct stfcrypto_softc *)self;
	struct fdt_attach_args *faa = aux;

	if (faa->fa_nreg < 1) {
		printf(": no registers\n");
		return;
	}

	sc->sc_iot = faa->fa_iot;
	if (bus_space_map(sc->sc_iot, faa->fa_reg[0].addr, faa->fa_reg[0].size,
		0, &sc->sc_ioh)) {
		printf(": can't map registers\n");
		return;
	}

	printf("\n");

	/* Enable clocks */
	clock_enable(faa->fa_node, "hclk");
	clock_enable(faa->fa_node, "ahb");
	reset_deassert(faa->fa_node, NULL);

	/* Initialize mutex */
	mtx_init(&sc->sc_mtx, IPL_NET);

	/* Setup crypto framework */
	sc->sc_cid = crypto_get_driverid(0);
	if (sc->sc_cid < 0) {
		printf(": can't get crypto driver id\n");
		goto fail;
	}

	memset(algs, 0, sizeof(algs));

	// TODO: Register algorithms

	return;

fail:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, faa->fa_reg[0].size);
}