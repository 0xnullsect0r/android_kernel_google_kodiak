// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright 2021-2025 Google LLC
 *
 * Override some functionality from upstream pcie-designware-host driver.
 * Ideally, code in here is structured such that it may eventually fit
 * within the upstream driver if/when the underlying hardware driver/feature
 * can be integrated upstream.
 */

#include <linux/irq.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/irqdesc.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/pm_runtime.h>
#include <linux/platform_device.h>
#include "pcie-designware-host-customized.h"
#include "pcie-google.h"

#define PCI_EXT_CAP_ID_PL_64GT	0x31	/* Physical Layer 64.0 GT/s */

/* Secondary PCIe Capability 8.0 GT/s */
#define PCI_SECPCI_LE_CTRL	0x0c	/* Lane Equalization Control Register */

/* Physical Layer 32.0 GT/s */
#define PCI_PL_32GT_LE_CTRL	0x20	/* Lane Equalization Control Register */

/* Physical Layer 64.0 GT/s */
#define PCI_PL_64GT_LE_CTRL	0x20	/* Lane Equalization Control Register */

/*
 * Some chips are configured for edge-triggered interrupts,
 * but the underlying signal is level-based. So with multiple
 * msi interrupts come in back to back, there is possibility
 * that the interrupt signal didn't goes low to high to create
 * edge for each and every incoming msi to allow GIC layer to
 * detect. To avoid that, we have this workaround to set irqchip
 * state to PENDING if necessary.
 */
static int goog_pci_check_if_need_retrigger(int msi_ctrl, struct dw_pcie_rp *pp)
{
	int err = 0, virq;
	unsigned long flags;
	u32 status, mask, irq_type;
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);

	raw_spin_lock_irqsave(&pp->lock, flags);
	virq = pp->msi_irq[msi_ctrl];
	irq_type = irq_get_trigger_type(virq);
	if (!(irq_type & IRQ_TYPE_EDGE_BOTH))
		goto unlock;
	if (pm_runtime_get_if_active(pci->dev) <= 0)
		goto unlock;
	status = dw_pcie_readl_dbi(pci, PCIE_MSI_INTR0_STATUS +
					   (msi_ctrl * MSI_REG_CTRL_BLOCK_SIZE));
	mask = pp->irq_mask[msi_ctrl];
	pm_runtime_put(pci->dev);

	status &= ~mask;
	if (!status)
		goto unlock;
	err = irq_set_irqchip_state(virq, IRQCHIP_STATE_PENDING, true);
unlock:
	raw_spin_unlock_irqrestore(&pp->lock, flags);
	return err;
}

/* MSI int handler */
static irqreturn_t goog_handle_msi_irq(struct dw_pcie_rp *pp)
{
	int i, pos, err;
	unsigned long val;
	u32 status, num_ctrls;
	irqreturn_t ret = IRQ_NONE;
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);

	num_ctrls = pp->num_vectors / MAX_MSI_IRQS_PER_CTRL;

	for (i = 0; i < num_ctrls; i++) {
		status = dw_pcie_readl_dbi(pci, PCIE_MSI_INTR0_STATUS +
					   (i * MSI_REG_CTRL_BLOCK_SIZE));
		if (!status)
			continue;

		ret = IRQ_HANDLED;
		val = status;
		pos = 0;
		while ((pos = find_next_bit(&val, MAX_MSI_IRQS_PER_CTRL,
					    pos)) != MAX_MSI_IRQS_PER_CTRL) {
			generic_handle_domain_irq(pp->irq_domain,
						  (i * MAX_MSI_IRQS_PER_CTRL) +
						  pos);
			pos++;
		}

		err = goog_pci_check_if_need_retrigger(i, pp);
		if (err)
			dev_err(pci->dev, "Failed to set irqchip state %d\n", err);
	}

	return ret;
}

static void goog_chained_msi_isr(struct irq_desc *desc)
{
	struct irq_chip *chip = irq_desc_get_chip(desc);
	struct dw_pcie_rp *pp;

	chained_irq_enter(chip, desc);

	pp = irq_desc_get_handler_data(desc);
	goog_handle_msi_irq(pp);

	chained_irq_exit(chip, desc);
}

void goog_setup_chained_irq_handler(struct dw_pcie_rp *pp)
{
	u32 ctrl, num_ctrls;

	num_ctrls = pp->num_vectors / MAX_MSI_IRQS_PER_CTRL;
	for (ctrl = 0; ctrl < num_ctrls; ctrl++) {
		if (pp->msi_irq[ctrl] > 0)
			irq_set_chained_handler_and_data(pp->msi_irq[ctrl],
							goog_chained_msi_isr, pp);
	}
}

static void goog_pci_bottom_ack(struct irq_data *d)
{
	struct dw_pcie_rp *pp  = irq_data_get_irq_chip_data(d);
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	unsigned int res, bit, ctrl;

	ctrl = d->hwirq / MAX_MSI_IRQS_PER_CTRL;
	res = ctrl * MSI_REG_CTRL_BLOCK_SIZE;
	bit = d->hwirq % MAX_MSI_IRQS_PER_CTRL;

	dw_pcie_writel_dbi(pci, PCIE_MSI_INTR0_STATUS + res, BIT(bit));
}

static void goog_pci_setup_msi_msg(struct irq_data *d, struct msi_msg *msg)
{
	struct dw_pcie_rp *pp = irq_data_get_irq_chip_data(d);
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	u64 msi_target;

	msi_target = (u64)pp->msi_data;

	msg->address_lo = lower_32_bits(msi_target);
	msg->address_hi = upper_32_bits(msi_target);

	msg->data = d->hwirq;

	dev_dbg(pci->dev, "msi#%d address_hi %#x address_lo %#x\n",
		(int)d->hwirq, msg->address_hi, msg->address_lo);
}

static void goog_pci_bottom_mask(struct irq_data *d)
{
	struct dw_pcie_rp *pp = irq_data_get_irq_chip_data(d);
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	unsigned int res, bit, ctrl;
	unsigned long flags;

	raw_spin_lock_irqsave(&pp->lock, flags);

	ctrl = d->hwirq / MAX_MSI_IRQS_PER_CTRL;
	res = ctrl * MSI_REG_CTRL_BLOCK_SIZE;
	bit = d->hwirq % MAX_MSI_IRQS_PER_CTRL;

	pp->irq_mask[ctrl] |= BIT(bit);
	dw_pcie_writel_dbi(pci, PCIE_MSI_INTR0_MASK + res, pp->irq_mask[ctrl]);

	raw_spin_unlock_irqrestore(&pp->lock, flags);
}

void goog_pci_bottom_unmask(struct irq_data *d)
{
	struct dw_pcie_rp *pp = irq_data_get_irq_chip_data(d);
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	unsigned int res, bit, ctrl;
	unsigned long flags;
	int err;

	raw_spin_lock_irqsave(&pp->lock, flags);

	ctrl = d->hwirq / MAX_MSI_IRQS_PER_CTRL;
	res = ctrl * MSI_REG_CTRL_BLOCK_SIZE;
	bit = d->hwirq % MAX_MSI_IRQS_PER_CTRL;

	pp->irq_mask[ctrl] &= ~BIT(bit);
	dw_pcie_writel_dbi(pci, PCIE_MSI_INTR0_MASK + res, pp->irq_mask[ctrl]);

	raw_spin_unlock_irqrestore(&pp->lock, flags);

	/* We may have missed an MSI edge while masked. */
	err = goog_pci_check_if_need_retrigger(ctrl, pp);
	if (err)
		dev_err(pci->dev, "Failed to set irqchip state %d\n", err);
}

static void dw_msi_ack_irq(struct irq_data *d)
{
	irq_chip_ack_parent(d);
}

static void dw_msi_mask_irq(struct irq_data *d)
{
	pci_msi_mask_irq(d);
	irq_chip_mask_parent(d);
}

static void dw_msi_unmask_irq(struct irq_data *d)
{
	pci_msi_unmask_irq(d);
	irq_chip_unmask_parent(d);
}

static struct irq_chip dw_pcie_msi_irq_chip = {
	.name = "PCI-MSI",
	.irq_ack = dw_msi_ack_irq,
	.irq_mask = dw_msi_mask_irq,
	.irq_unmask = dw_msi_unmask_irq,
};

static struct msi_domain_info dw_pcie_msi_domain_info = {
	.flags	= MSI_FLAG_USE_DEF_DOM_OPS | MSI_FLAG_USE_DEF_CHIP_OPS |
		  MSI_FLAG_PCI_MSIX | MSI_FLAG_MULTI_PCI_MSI,
	.chip	= &dw_pcie_msi_irq_chip,
};

/*
 * Duped with implementation in the pcie-google.c, need to restruct later.
 */
static int dw_pcie_irq_domain_alloc(struct irq_domain *domain,
				    unsigned int virq, unsigned int nr_irqs,
				    void *args)
{
	struct dw_pcie_rp *pp = domain->host_data;
	unsigned long flags;
	u32 i;
	int bit;

	raw_spin_lock_irqsave(&pp->lock, flags);

	bit = bitmap_find_free_region(pp->msi_irq_in_use, pp->num_vectors,
				      order_base_2(nr_irqs));

	raw_spin_unlock_irqrestore(&pp->lock, flags);

	if (bit < 0)
		return -ENOSPC;

	for (i = 0; i < nr_irqs; i++)
		irq_domain_set_info(domain, virq + i, bit + i,
				    pp->msi_irq_chip,
				    pp, handle_edge_irq,
				    NULL, NULL);

	return 0;
}

static void dw_pcie_irq_domain_free(struct irq_domain *domain,
				    unsigned int virq, unsigned int nr_irqs)
{
	struct irq_data *d = irq_domain_get_irq_data(domain, virq);
	struct dw_pcie_rp *pp = domain->host_data;
	unsigned long flags;

	raw_spin_lock_irqsave(&pp->lock, flags);

	bitmap_release_region(pp->msi_irq_in_use, d->hwirq,
			      order_base_2(nr_irqs));

	raw_spin_unlock_irqrestore(&pp->lock, flags);
}

static const struct irq_domain_ops dw_pcie_msi_domain_ops = {
	.alloc	= dw_pcie_irq_domain_alloc,
	.free	= dw_pcie_irq_domain_free,
};

static struct irq_chip goog_pci_msi_bottom_irq_chip = {
	.name = "DWPCI-MSI",
	.irq_ack = goog_pci_bottom_ack,
	.irq_compose_msi_msg = goog_pci_setup_msi_msg,
	.irq_mask = goog_pci_bottom_mask,
	.irq_unmask = goog_pci_bottom_unmask,
};

static void goog_pcie_free_msi(struct dw_pcie_rp *pp)
{
	u32 ctrl;

	for (ctrl = 0; ctrl < MAX_MSI_CTRLS; ctrl++) {
		if (pp->msi_irq[ctrl] > 0)
			irq_set_chained_handler_and_data(pp->msi_irq[ctrl],
							 NULL, NULL);
	}

	irq_domain_remove(pp->msi_domain);
	irq_domain_remove(pp->irq_domain);
}

static int goog_pcie_allocate_domains(struct dw_pcie_rp *pp)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct fwnode_handle *fwnode = of_node_to_fwnode(pci->dev->of_node);

	pp->irq_domain = irq_domain_create_linear(fwnode, pp->num_vectors,
					       &dw_pcie_msi_domain_ops, pp);
	if (!pp->irq_domain) {
		dev_err(pci->dev, "Failed to create IRQ domain\n");
		return -ENOMEM;
	}

	irq_domain_update_bus_token(pp->irq_domain, DOMAIN_BUS_NEXUS);

	pp->msi_domain = pci_msi_create_irq_domain(fwnode,
						   &dw_pcie_msi_domain_info,
						   pp->irq_domain);
	if (!pp->msi_domain) {
		dev_err(pci->dev, "Failed to create MSI domain\n");
		irq_domain_remove(pp->irq_domain);
		return -ENOMEM;
	}

	return 0;
}

static int goog_pcie_parse_split_msi_irq(struct dw_pcie_rp *pp)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct device *dev = pci->dev;
	struct platform_device *pdev = to_platform_device(dev);
	u32 ctrl, max_vectors;
	int irq;

	/* Parse any "msiX" IRQs described in the devicetree */
	for (ctrl = 0; ctrl < MAX_MSI_CTRLS; ctrl++) {
		char msi_name[] = "msiX";

		msi_name[3] = '0' + ctrl;
		irq = platform_get_irq_byname_optional(pdev, msi_name);
		if (irq == -ENXIO)
			break;
		if (irq < 0)
			return dev_err_probe(dev, irq,
					     "Failed to parse MSI IRQ '%s'\n",
					     msi_name);

		pp->msi_irq[ctrl] = irq;
	}

	/* If no "msiX" IRQs, caller should fallback to "msi" IRQ */
	if (ctrl == 0)
		return -ENXIO;

	max_vectors = ctrl * MAX_MSI_IRQS_PER_CTRL;
	if (pp->num_vectors > max_vectors)
		dev_warn(dev, "Exceeding number of MSI vectors, limiting to %u\n",
			 max_vectors);

	pp->num_vectors = max_vectors;
	return 0;
}

int goog_pcie_msi_host_init(struct dw_pcie_rp *pp)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct device *dev = pci->dev;
	struct platform_device *pdev = to_platform_device(dev);
	u64 *msi_vaddr = NULL;
	int ret;
	u32 ctrl, num_ctrls;

	/*
	 * While we're extending some functionality, we still want the DWC driver to
	 * think this is a standard iMSI-RX controller.
	 */
	pp->has_msi_ctrl = true;

	for (ctrl = 0; ctrl < MAX_MSI_CTRLS; ctrl++)
		pp->irq_mask[ctrl] = ~0;

	if (!pp->msi_irq[0]) {
		ret = goog_pcie_parse_split_msi_irq(pp);
		if (ret < 0 && ret != -ENXIO)
			return ret;
	}

	if (!pp->num_vectors)
		pp->num_vectors = MSI_DEF_NUM_VECTORS;
	num_ctrls = pp->num_vectors / MAX_MSI_IRQS_PER_CTRL;

	if (!pp->msi_irq[0]) {
		pp->msi_irq[0] = platform_get_irq_byname_optional(pdev, "msi");
		if (pp->msi_irq[0] < 0) {
			pp->msi_irq[0] = platform_get_irq(pdev, 0);
			if (pp->msi_irq[0] < 0)
				return pp->msi_irq[0];
		}
	}

	dev_dbg(dev, "Using %d MSI vectors\n", pp->num_vectors);

	pp->msi_irq_chip = &goog_pci_msi_bottom_irq_chip;

	ret = goog_pcie_allocate_domains(pp);
	if (ret)
		return ret;

	for (ctrl = 0; ctrl < num_ctrls; ctrl++) {
		if (pp->msi_irq[ctrl] > 0)
			irq_set_chained_handler_and_data(pp->msi_irq[ctrl],
						    goog_chained_msi_isr, pp);
	}

	/*
	 * Even though the iMSI-RX Module supports 64-bit addresses some
	 * peripheral PCIe devices may lack 64-bit message support. In
	 * order not to miss MSI TLPs from those devices the MSI target
	 * address has to be within the lowest 4GB.
	 *
	 * Note until there is a better alternative found the reservation is
	 * done by allocating from the artificially limited DMA-coherent
	 * memory.
	 */
	ret = dma_set_coherent_mask(dev, DMA_BIT_MASK(32));
	if (!ret)
		msi_vaddr = dmam_alloc_coherent(dev, sizeof(u64), &pp->msi_data,
						GFP_KERNEL);

	if (!msi_vaddr) {
		dev_warn(dev, "Failed to allocate 32-bit MSI address\n");
		dma_set_coherent_mask(dev, DMA_BIT_MASK(64));
		msi_vaddr = dmam_alloc_coherent(dev, sizeof(u64), &pp->msi_data,
						GFP_KERNEL);
		if (!msi_vaddr) {
			dev_err(dev, "Failed to allocate MSI address\n");
			goog_pcie_free_msi(pp);
			return -ENOMEM;
		}
	}

	return 0;
}

int dw_pcie_link_get_max_link_width(struct dw_pcie *pci)
{
	u8 cap = dw_pcie_find_capability(pci, PCI_CAP_ID_EXP);
	u32 lnkcap = dw_pcie_readl_dbi(pci, cap + PCI_EXP_LNKCAP);

	return FIELD_GET(PCI_EXP_LNKCAP_MLW, lnkcap);
}

/**
 * of_pci_get_equalization_presets - Parses the "eq-presets-Ngts" property.
 *
 * @dev: Device containing the properties.
 * @presets: Pointer to store the parsed data.
 * @num_lanes: Maximum number of lanes supported.
 *
 * If the property is present, read and store the data in the @presets structure.
 * Else, assign a default value of PCI_EQ_RESV.
 *
 * Return: 0 if the property is not available or successfully parsed else
 * errno otherwise.
 */
int of_pci_get_equalization_presets(struct device *dev,
				    struct pci_eq_presets *presets,
				    int num_lanes)
{
	char name[20];
	int ret;

	presets->eq_presets_8gts[0] = PCI_EQ_RESV;
	ret = of_property_read_u16_array(dev->of_node, "eq-presets-8gts",
					 presets->eq_presets_8gts, num_lanes);
	if (ret && ret != -EINVAL) {
		dev_err(dev, "Error reading eq-presets-8gts: %d\n", ret);
		return ret;
	}

	for (int i = 0; i < EQ_PRESET_TYPE_MAX - 1; i++) {
		presets->eq_presets_Ngts[i][0] = PCI_EQ_RESV;
		snprintf(name, sizeof(name), "eq-presets-%dgts", 8 << (i + 1));
		ret = of_property_read_u8_array(dev->of_node, name,
						presets->eq_presets_Ngts[i],
						num_lanes);
		if (ret && ret != -EINVAL) {
			dev_err(dev, "Error reading %s: %d\n", name, ret);
			return ret;
		}
	}

	return 0;
}

static void dw_pcie_program_presets(struct dw_pcie_rp *pp, enum pci_bus_speed speed)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct google_pcie *gpcie = dev_get_drvdata(pci->dev);
	u8 lane_eq_offset, lane_reg_size, cap_id;
	u8 *presets;
	u32 cap;
	int i;

	if (speed == PCIE_SPEED_8_0GT) {
		presets = (u8 *)gpcie->presets.eq_presets_8gts;
		lane_eq_offset =  PCI_SECPCI_LE_CTRL;
		cap_id = PCI_EXT_CAP_ID_SECPCI;
		/* For data rate of 8 GT/S each lane equalization control is 16bits wide*/
		lane_reg_size = 0x2;
	} else if (speed == PCIE_SPEED_16_0GT) {
		presets = gpcie->presets.eq_presets_Ngts[EQ_PRESET_TYPE_16GTS - 1];
		lane_eq_offset = PCI_PL_16GT_LE_CTRL;
		cap_id = PCI_EXT_CAP_ID_PL_16GT;
		lane_reg_size = 0x1;
	} else if (speed == PCIE_SPEED_32_0GT) {
		presets =  gpcie->presets.eq_presets_Ngts[EQ_PRESET_TYPE_32GTS - 1];
		lane_eq_offset = PCI_PL_32GT_LE_CTRL;
		cap_id = PCI_EXT_CAP_ID_PL_32GT;
		lane_reg_size = 0x1;
	} else if (speed == PCIE_SPEED_64_0GT) {
		presets =  gpcie->presets.eq_presets_Ngts[EQ_PRESET_TYPE_64GTS - 1];
		lane_eq_offset = PCI_PL_64GT_LE_CTRL;
		cap_id = PCI_EXT_CAP_ID_PL_64GT;
		lane_reg_size = 0x1;
	} else {
		return;
	}

	if (presets[0] == PCI_EQ_RESV)
		return;

	cap = dw_pcie_find_ext_capability(pci, cap_id);
	if (!cap)
		return;

	/*
	 * Write preset values to the registers byte-by-byte for the given
	 * number of lanes and register size.
	 */
	for (i = 0; i < pci->num_lanes * lane_reg_size; i++)
		dw_pcie_writeb_dbi(pci, cap + lane_eq_offset + i, presets[i]);
}

void dw_pcie_config_presets(struct dw_pcie_rp *pp)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	enum pci_bus_speed speed = pcie_link_speed[pci->max_link_speed];

	/*
	 * Lane equalization settings need to be applied for all data rates the
	 * controller supports and for all supported lanes.
	 */

	if (speed >= PCIE_SPEED_8_0GT)
		dw_pcie_program_presets(pp, PCIE_SPEED_8_0GT);

	if (speed >= PCIE_SPEED_16_0GT)
		dw_pcie_program_presets(pp, PCIE_SPEED_16_0GT);

	if (speed >= PCIE_SPEED_32_0GT)
		dw_pcie_program_presets(pp, PCIE_SPEED_32_0GT);

	if (speed >= PCIE_SPEED_64_0GT)
		dw_pcie_program_presets(pp, PCIE_SPEED_64_0GT);
}
