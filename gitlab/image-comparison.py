import sys
import argparse
from pathlib import Path
import os
import skimage.io
import skimage.metrics
import skimage.color
import skimage.util
import skimage.transform
import skimage.filters
import numpy as np
import itertools
from sewar.full_ref import vifp, uqi


def evaluate_metrics(reference, candidate):
    return {
        # Structural similarity index between two images
        # (humans are better at perceiving structural differences than subtle pixel changes).
        # it compares the images based on luminance Contast and structure.
        # ssim provides much better results when applied to small patches of the image than on its entirety. Typically an 11x11 window is used.
        # ssim provides scores from -1 (not similar) to 1 (same image).
        # ssim provides high scores one of the images is blured or the color space is shifted
        # further reading:
        # https://en.wikipedia.org/wiki/Structural_similarity
        # "ssim": skimage.metrics.structural_similarity(reference, candidate, multichannel=True),
        
        # Mean Squared Error (MSE) is one of the most commonly used image quality measures, but receives strong criticism as
        # it doesn't reflect the way the human visual systems perceive images very well. An image pair with
        # high MSE might still look very similar to a human.
        "mse": skimage.metrics.mean_squared_error(reference, candidate),
        # Peak Signal to Noise Ratio (PSNR) is based on MSE and brought to a logarithmic scale in the decibel unit
        #"psnr": skimage.metrics.peak_signal_noise_ratio(reference, candidate),
        
        # Normalized Root MSE (NRMSE)
        # "nrmse": skimage.metrics.normalized_root_mse(reference, candidate),
        
        # Mean Absolute Error (MAE)
        # "mae": np.absolute(np.subtract(reference, candidate)).mean(),
        
        # Visual Information Fidelity Measure (VIF)
        # https://ieeexplore.ieee.org/abstract/document/1576816/
        # The higher the better. The reference image would yield a value of 1.0, values above 1.0
        # typically stem from higher contrast images, which are considerered better quality in this metric
        # "vif": vifp(reference, candidate),
        
        # Universal Quality Index (UQI)
        # "uqi": uqi(reference, candidate),
    }
    
def evaluate_passed(metrics, threshold):
	# Through survey and analysis new threshold values were determined for ssim and psnr.
	# These are the values in the code. The original source for these values is in thresholds.csv.
	# The initial values were 0.85 and 20.0.
	# Leonard Daly, 2021-10-07
    return {
        # Choose a relaxed value for SSIM
        # "ssim": metrics["ssim"] > 0.70,
        # PSNR for image compression in 8bit is typically in the range [30, 50]
        # "psnr": metrics["psnr"] > 20.0, 
        # MSE 
        "mse": metrics["mse"] > threshold
    }

def print_report(metrics_report):

    for name, value in metrics_report["metrics"].items():
        passed = ''
        if name in metrics_report['passed']:
            passed = f"[{'Passed' if metrics_report['passed'][name] else 'Failed'}]"
        print(f"{name: <8} {value :<12.6f} {passed}")

    print("")

def compare_images(reference, candidate):
    diff = skimage.util.compare_images(reference, candidate, method='diff')
    threshold = diff > 0.05

    return {
        "reference": reference,
        "candidate": candidate,
        "diff": skimage.util.img_as_ubyte(diff),
        "threshold": skimage.util.img_as_ubyte(threshold)
    }

def evaluate(reference, candidate, threshold):
    metrics = evaluate_metrics(reference, candidate)

    return {
        "metrics": metrics,
        "passed": evaluate_passed(metrics, threshold),
        "images": compare_images(reference, candidate),
    }

def normalize_images(reference, candidate):
    # Ensure images match in channel count
    if reference.shape[2] == 4:
        reference = skimage.color.rgba2rgb(reference)
        reference = skimage.util.img_as_ubyte(reference)
    if candidate.shape[2] == 4:
        candidate = skimage.color.rgba2rgb(candidate)
        candidate = skimage.util.img_as_ubyte(candidate)
    # Ensure images match in resolution
    if candidate.shape != reference.shape:
        print(f"\n  Resizing {candidate_path} from {candidate.shape[:2]} to {reference.shape[:2]}")
        candidate = skimage.transform.resize(candidate, (reference.shape[0], reference.shape[1]),
                    anti_aliasing=False)
        candidate = skimage.util.img_as_ubyte(candidate)
        print()
    return reference, candidate

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Compare two images with MSE.')
    parser.add_argument("--reference", "-g", help="Gold image for comparison", default="")
    parser.add_argument("--candidate", "-c", help="Image to be compared", default="")
    parser.add_argument("--mse", "-m", help="Use mse to measure difference.")
    args = parser.parse_args()

    reference_path = Path(args.reference)
    candidate_path = Path(args.candidate)
    # scan the filesystem for test case images

    results = {}
    reference_image = skimage.io.imread(reference_path)
    candidate_image = skimage.util.img_as_ubyte(skimage.io.imread(candidate_path))
    reference_image, candidate_image = normalize_images(reference_image, candidate_image)

    # Compute metrics and compare images
    results = evaluate(reference_image, candidate_image, float(args.mse))
    if float(results["metrics"]["mse"]) > float(args.mse):
        print("Failure: MSE " + str(results["metrics"]["mse"]) + " is greater than the threshold " + args.mse)
        sys.exit(-1)
    else:
        print("Success: MSE " + str(results["metrics"]["mse"]) + " is less than the threshold " + args.mse)
        sys.exit(0)
    
    # CLI output
    #print_report(results)
    #sys.exit(float(results["metrics"]["mse"]) > float(args.mse))
    
