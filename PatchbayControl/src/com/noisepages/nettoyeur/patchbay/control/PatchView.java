package com.noisepages.nettoyeur.patchbay.control;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

import android.app.Notification;
import android.app.PendingIntent.CanceledException;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.os.RemoteException;
import android.util.AttributeSet;
import android.util.Log;
import android.util.Pair;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ToggleButton;

import com.noisepages.nettoyeur.patchbay.IPatchbayService;

public final class PatchView extends FrameLayout {

  private IPatchbayService patchbay;
  private final List<String> modules = new ArrayList<String>();
  private final Map<String, Integer> inputs = new HashMap<String, Integer>();
  private final Map<String, Integer> outputs = new HashMap<String, Integer>();
  private final Map<String, Notification> notifications = new HashMap<String, Notification>();
  private final Map<Pair<String, Integer>, List<Pair<String, Integer>>> connections =
      new HashMap<Pair<String, Integer>, List<Pair<String, Integer>>>();
  
  private final Map<String, LinearLayout> moduleViews = new HashMap<String, LinearLayout>();

  private PatchOverlay overlay = null;
  private ToggleButton selectedButton = null;
  private String selectedModule = null;
  private int selectedInput = -1;;
  private int selectedOutput = -1;;

  public PatchView(Context context) {
    super(context);
    init();
  }

  public PatchView(Context context, AttributeSet attrs) {
    super(context, attrs);
    init();
  }

  public PatchView(Context context, AttributeSet attrs, int defStyle) {
    super(context, attrs, defStyle);
    init();
  }

  private void init() {
  }

  public void setPatchbay(IPatchbayService patchbay) {
    this.patchbay = patchbay;
  }

  public void addModule(String module, int inputChannels, int outputChannels,
      Notification notification) {
    for (String u : modules) {
      int sinks = 0;
      try {
        sinks = patchbay.getInputChannels(u);
      } catch (RemoteException e) {
        e.printStackTrace();
        continue;
      }
      for (int i = 0; i < outputChannels; ++i) {
        for (int j = 0; j < sinks; ++j) {
          try {
            if (patchbay.isConnected(module, i, u, j)) {
              addConnection(module, i, u, j);
            }
          } catch (RemoteException e) {
            e.printStackTrace();
          }
        }
      }
      int sources = 0;
      try {
        sources = patchbay.getOutputChannels(u);
      } catch (RemoteException e) {
        e.printStackTrace();
        continue;
      }
      for (int i = 0; i < inputChannels; ++i) {
        for (int j = 0; j < sources; ++j) {
          try {
            if (patchbay.isConnected(u, j, module, i)) {
              addConnection(u, j, module, i);
            }
          } catch (RemoteException e) {
            e.printStackTrace();
          }
        }
      }
    }
    modules.add(module);
    inputs.put(module, inputChannels);
    outputs.put(module, outputChannels);

    if (notification == null) {
      notification =
          new Notification.Builder(getContext()).setContentTitle(module)
              .setSmallIcon(android.R.drawable.ic_media_play).build();
    }
    notifications.put(module, notification);
    addModuleView(module, inputChannels, outputChannels, notification);
  }

  private void addModuleView(final String module, int inputChannels, int outputChannels,
      final Notification notification) {
    if (overlay == null) {
      overlay = (PatchOverlay) getChildAt(1);
      overlay.setPatchView(this);
    }
    LinearLayout moduleList = (LinearLayout) getChildAt(0);
    LinearLayout moduleView = (LinearLayout) inflate(getContext(), R.layout.module, null);
    moduleList.addView(moduleView);
    moduleViews.put(module, moduleView);

    LinearLayout buttonLayout = (LinearLayout) moduleView.getChildAt(0);
    List<Button> buttons = new ArrayList<Button>();
    for (int i = 0; i < inputChannels; ++i) {
      final ToggleButton button = new ToggleButton(getContext());
      button.setTextOn("");
      button.setTextOff("");
      button.setChecked(false);
      buttons.add(button);
      buttonLayout.addView(button);
      final int j = i;
      button.setOnCheckedChangeListener(new OnCheckedChangeListener() {
        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
          if (isChecked) {
            if (selectedButton == null) {
              selectedButton = button;
              selectedModule = module;
              selectedInput = j;
              return;
            } else if (selectedOutput >= 0) {
              try {
                if (patchbay.isConnected(selectedModule, selectedOutput, module, j)) {
                  patchbay.disconnectModules(selectedModule, selectedOutput, module, j);
                } else {
                  patchbay.connectModules(selectedModule, selectedOutput, module, j);
                }
              } catch (RemoteException e) {
                e.printStackTrace();
              }
            }
          }
          button.setChecked(false);
          if (selectedButton != null) {
            selectedButton.setChecked(false);
            selectedButton = null;
            selectedModule = null;
            selectedInput = -1;
            selectedOutput = -1;
          }
        }
      });
    }

    FrameLayout frame = (FrameLayout) moduleView.getChildAt(1);
    View view = notification.contentView.apply(getContext(), frame);
    view.setOnLongClickListener(new OnLongClickListener() {
      @Override
      public boolean onLongClick(View v) {
        try {
          if (patchbay.isActive(module)) {
            patchbay.deactivateModule(module);
          } else {
            patchbay.activateModule(module);
          }
        } catch (RemoteException e) {
          e.printStackTrace();
        }
        return true;
      }
    });
    if (notification.contentIntent != null) {
      view.setOnClickListener(new OnClickListener() {
        @Override
        public void onClick(View v) {
          try {
            notification.contentIntent.send();
          } catch (CanceledException e) {
            e.printStackTrace();
          }
        }
      });
    }
    frame.addView(view);

    buttonLayout = (LinearLayout) moduleView.getChildAt(2);
    buttons = new ArrayList<Button>();
    for (int i = 0; i < outputChannels; ++i) {
      final ToggleButton button = new ToggleButton(getContext());
      button.setTextOn("");
      button.setTextOff("");
      button.setChecked(false);
      buttons.add(button);
      buttonLayout.addView(button);
      final int j = i;
      button.setOnCheckedChangeListener(new OnCheckedChangeListener() {
        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
          if (isChecked) {
            if (selectedButton == null) {
              selectedButton = button;
              selectedModule = module;
              selectedOutput = j;
              return;
            } else if (selectedInput >= 0) {
              try {
                if (patchbay.isConnected(module, j, selectedModule, selectedInput)) {
                  patchbay.disconnectModules(module, j, selectedModule, selectedInput);
                } else {
                  patchbay.connectModules(module, j, selectedModule, selectedInput);
                }
              } catch (RemoteException e) {
                e.printStackTrace();
              }
            }
          }
          button.setChecked(false);
          if (selectedButton != null) {
            selectedButton.setChecked(false);
            selectedButton = null;
            selectedModule = null;
            selectedInput = -1;
            selectedOutput = -1;
          }
        }
      });
    }

    invalidateAll();
  }

  public void deleteModule(String module) {
    for (Iterator<Entry<Pair<String, Integer>, List<Pair<String, Integer>>>> iter =
        connections.entrySet().iterator(); iter.hasNext();) {
      Entry<Pair<String, Integer>, List<Pair<String, Integer>>> entry = iter.next();
      if (entry.getKey().first.equals(module)) {
        iter.remove();
      } else {
        for (Iterator<Pair<String, Integer>> iter2 = entry.getValue().iterator(); iter2.hasNext();) {
          Pair<String, Integer> q = iter2.next();
          if (q.first.equals(module)) {
            iter2.remove();
          }
        }
      }
    }
    modules.remove(module);
    inputs.remove(module);
    outputs.remove(module);
    notifications.remove(module);
    deleteModuleView(module);
  }

  private void deleteModuleView(String module) {
    LinearLayout moduleList = (LinearLayout) getChildAt(0);
    View moduleView = moduleViews.remove(module);
    if (moduleView != null) {
      moduleList.removeView(moduleView);
    }
    invalidateAll();
  }

  public void addConnection(String source, int sourcePort, String sink, int sinkPort) {
    Pair<String, Integer> a = new Pair<String, Integer>(source, sourcePort);
    List<Pair<String, Integer>> c = connections.get(a);
    if (c == null) {
      c = new ArrayList<Pair<String, Integer>>();
      connections.put(a, c);
    }
    Pair<String, Integer> b = new Pair<String, Integer>(sink, sinkPort);
    if (!c.contains(b)) {
      c.add(b);
    }
    invalidateAll();
  }

  public void removeConnection(String source, int sourcePort, String sink, int sinkPort) {
    Pair<String, Integer> a = new Pair<String, Integer>(source, sourcePort);
    List<Pair<String, Integer>> c = connections.get(a);
    if (c == null) {
      return;
    }
    Pair<String, Integer> b = new Pair<String, Integer>(sink, sinkPort);
    c.remove(b);
    invalidateAll();
  }

  void drawOverlay(Canvas canvas) {
    int a[] = new int[4];
    int b[] = new int[4]; 
    Paint circlePaint = new Paint();
    circlePaint.setColor(Color.RED);
    circlePaint.setStrokeWidth(10);
    circlePaint.setStyle(Paint.Style.FILL);
    Paint paint = new Paint();
    paint.setAntiAlias(true);
    paint.setColor(Color.RED);
    paint.setStrokeWidth(10);
    paint.setStyle(Paint.Style.STROKE);
    int N = 3;
    for (Pair<String, Integer> p : connections.keySet()) {
      getOutputPortCoordinates(p.first, p.second, a);
      int x0 = (a[0] + a[2]) / 2;
      int y0 = (a[1] + a[3]) / 2;
      for (Pair<String, Integer> q : connections.get(p)) {
        getInputPortCoordinates(q.first, q.second, b);
        int x1 = (b[0] + b[2]) / 2;
        int y1 = (b[1] + b[3]) / 2;
        Path path = new Path();
        path.moveTo(x0, y0);
        path.cubicTo((x0 * (N - 1) + x1) / N, y0, (x0 + x1 * (N - 1)) / N, y1, x1, y1);
        canvas.drawPath(path, paint);
        canvas.drawCircle(x0, y0, 20, circlePaint);
        canvas.drawCircle(x1, y1, 20, circlePaint);
        Log.i("onDraw", "x0: " + x0 + ", y0: " + y0 + ", x1: " + x1 + ", y1: " + y1);
      }
    }
  }

  private void getInputPortCoordinates(String module, int i, int[] r) {
    getCoordinates(module, i, r, 0);
  }

  private void getOutputPortCoordinates(String module, int i, int[] r) {
    getCoordinates(module, i, r, 2);
  }

  private void getCoordinates(String module, int i, int[] r, int j) {
    LinearLayout mv = moduleViews.get(module);
    LinearLayout row = (LinearLayout) mv.getChildAt(j);
    View button = row.getChildAt(i);
    int x = mv.getLeft() + row.getLeft() + button.getLeft();
    int y = mv.getTop() + row.getTop() + button.getTop();
    r[0] = x;
    r[1] = y;
    r[2] = x + button.getWidth();
    r[3] = y + button.getHeight();
  }
  
  private void invalidateAll() {
    invalidate();
    if (overlay != null) {
      overlay.invalidate();
    }
  }
}
