package com.uni.common.util;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ObjectAnimator;
import android.view.View;

/**
 * Created by ZachLi on 2016/9/7.
 */
public class AnimationUtil {


    public static void hideByAlpha(final View view, int duration) {

        ObjectAnimator alpha = ObjectAnimator.ofFloat(view, "alpha", 1.0f, 0.5f, 0.0f);
        alpha.setDuration(duration);
        alpha.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                super.onAnimationEnd(animation);
                view.setVisibility(View.GONE);
            }
        });
        alpha.start();
    }


    public static void showByAlpha(final View view, int duration) {

        ObjectAnimator alpha = ObjectAnimator.ofFloat(view, "alpha", 0.3f, 0.8f, 1.0f);
        alpha.setDuration(duration);
        alpha.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                super.onAnimationEnd(animation);
                view.setVisibility(View.VISIBLE);
            }
        });
        alpha.start();
    }

    public static void animateTop(final View view) {

        if (view.getVisibility() == View.VISIBLE) {
            view.animate().translationY(0)
                    .translationYBy(-view.getHeight())
                    .setDuration(200)
                    .setListener(new AnimatorListenerAdapter() {
                        @Override
                        public void onAnimationEnd(Animator animation) {
                            super.onAnimationEnd(animation);
                            view.setVisibility(View.GONE);
                        }
                    }).start();
        } else {
            view.animate().translationY(0)
                    .translationYBy(view.getHeight())
                    .setDuration(200)
                    .setListener(new AnimatorListenerAdapter() {
                        @Override
                        public void onAnimationStart(Animator animation) {
                            super.onAnimationStart(animation);
                            view.setVisibility(View.VISIBLE);
                        }
                    }).start();

        }
    }

    public static void animateBottom(final View view) {
        if (view.getVisibility() == View.VISIBLE) {
            view.animate().translationY(0)
                    .translationYBy(view.getHeight())
                    .setDuration(200)
                    .setListener(new AnimatorListenerAdapter() {
                        @Override
                        public void onAnimationEnd(Animator animation) {
                            super.onAnimationEnd(animation);
                            view.setVisibility(View.INVISIBLE);
                        }
                    }).start();

        } else {
            view.animate().translationY(0)
                    .translationYBy(-view.getHeight())
                    .setDuration(200)
                    .setListener(new AnimatorListenerAdapter() {
                        @Override
                        public void onAnimationStart(Animator animation) {
                            super.onAnimationEnd(animation);
                        }
                    }).start();
            view.setVisibility(View.VISIBLE);
        }
    }

    public static void animateLeft(final View view) {
        if (view.getVisibility() == View.VISIBLE) {
            view.animate().translationX(0)
                    .translationXBy(view.getWidth())
                    .setDuration(200)
                    .setListener(new AnimatorListenerAdapter() {
                        @Override
                        public void onAnimationEnd(Animator animation) {
                            super.onAnimationEnd(animation);
                            view.setVisibility(View.INVISIBLE);
                        }
                    }).start();

        } else {
            view.animate().translationX(0)
                    .translationXBy(-view.getWidth())
                    .setDuration(200)
                    .setListener(new AnimatorListenerAdapter() {
                        @Override
                        public void onAnimationStart(Animator animation) {
                            super.onAnimationEnd(animation);
                        }
                    }).start();
            view.setVisibility(View.VISIBLE);
        }
    }

    public static void animateRight(final View view) {
        if (view.getVisibility() == View.VISIBLE) {
            view.animate().translationX(0)
                    .translationXBy(view.getWidth())
                    .setDuration(200)
                    .setListener(new AnimatorListenerAdapter() {
                        @Override
                        public void onAnimationEnd(Animator animation) {
                            super.onAnimationEnd(animation);
                            view.setVisibility(View.INVISIBLE);
                        }
                    }).start();

        } else {
            view.animate().translationX(0)
                    .translationXBy(-view.getWidth())
                    .setDuration(200)
                    .setListener(new AnimatorListenerAdapter() {
                        @Override
                        public void onAnimationStart(Animator animation) {
                            super.onAnimationEnd(animation);
                        }
                    }).start();
            view.setVisibility(View.VISIBLE);
        }
    }
}
