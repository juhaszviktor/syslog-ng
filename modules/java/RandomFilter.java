/*
 * Copyright (c) 2014 BalaBit IT Ltd, Budapest, Hungary
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

package org.syslog_ng;

import java.util.Random;

public class RandomFilter extends FilterExprNode {

    private static Random random = new Random();

    public RandomFilter(long handle) {
        super(handle);
    }

    public boolean init() {
        return true;
    }

    public boolean eval(LogMessage msg) {
        InternalMessageSender.debug("Evaluating filter in Java");
        InternalMessageSender.debug("Name option: " + getOption("name"));
        msg.release();

        return random.nextBoolean();
    }

}
